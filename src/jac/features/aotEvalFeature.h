#pragma once

#include <jac/features/evalFeature.h>
#include <jac/machine/functionFactory.h>
#include <jac/machine/machine.h>

#include <jac/machine/compiler/ast.h>
#include <jac/machine/compiler/cfg.h>
#include <jac/machine/compiler/cfgEmit.h>
#include <jac/machine/compiler/cfgSimplify.h>
#include <jac/machine/compiler/scanner.h>
#include <jac/machine/compiler/traverseFuncs.h>

#include <list>

#include "BE/Base/ir.h"
#include "BE/Base/serialize.h"
#include "BE/CodeGenX64/codegen.h"
#include "BE/CodeGenX64/legalize.h"
#include "BE/CodeGenX64/regs.h"
#include "BE/CpuX64/assembler.h"
#include "Util/assert.h"
#include "Util/assert.h"
#include "Util/stripe.h"
#include "jac/machine/compiler/cfgCwerg.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <sys/mman.h>
#include <utility>
#include <vector>


namespace jac {


struct CompFn {
    std::string name;
    std::string alias;
    std::string_view code;
    std::optional<Function> jsFn;
};

namespace detail {


template<typename Res>
struct CompiledCallable {
    void* callerPtr;

    Res operator()(std::vector<ValueWeak> args) {
        std::vector<JSValue> jsArgs;
        for (ValueWeak& arg : args) {
            jsArgs.push_back(arg.getVal());
        }

        if constexpr (std::is_same_v<Res, bool>) {
            auto fn = reinterpret_cast<int32_t(*)(int64_t, JSValue*)>(callerPtr); // NOLINT
            return fn(jsArgs.size(), jsArgs.data());
        }
        else {
            auto fn = reinterpret_cast<Res(*)(int64_t, JSValue*)>(callerPtr); // NOLINT
            return fn(jsArgs.size(), jsArgs.data());
        }
    }
};


} // namespace detail


template<class Next>
class AotEvalFeature : public EvalFeature<Next> {
    std::map<std::string, CompFn> compiledHolder;

    std::string alias(const void* ptr) {
        return "__jac_aot_func_" + std::to_string(reinterpret_cast<uint64_t>(ptr));  // NOLINT
    }

    jac::ast::Script parseScript(std::string_view js) {
        bool hadError = false;
        std::vector<std::string> reports;
        jac::lex::Scanner scanner(js, [&hadError, &reports](int line, int col, const std::string& msg) {
            hadError = true;
            reports.push_back("Lex error: " + msg + " at " + std::to_string(line) + ":" + std::to_string(col));
        });

        if (hadError) {
            for (const auto& report : reports) {
                std::cerr << report << '\n';
            }
            throw std::runtime_error("Lex error");
        }

        auto tokens = scanner.scan();

        jac::ast::ParserState state(tokens);

        auto script = jac::ast::Script::parse(state);
        if (!script || !state.isEnd()) {
            lex::Token errorToken = state.getErrorToken();
            std::cerr << "Parse error: " << state.getErrorMessage()
                      << " at " << errorToken.line << ":" << errorToken.column << '\n';
            throw std::runtime_error("Parse error");
        }

        return std::move(*script);
    }

    std::list<std::pair<jac::cfg::Function, std::string_view>> transformFunctions(const jac::ast::traverse::Functions& functions) {
        std::list<std::pair<jac::cfg::Function, std::string_view>> result;
        std::map<cfg::Identifier, cfg::SignaturePtr> signatures;
        for (const auto& astFunc : functions.functions) {
            if (!astFunc->name) {
                continue;
            }
            auto sig = jac::cfg::getSignature(*astFunc);
            if (sig) {
                signatures.emplace(astFunc->name->identifier.name.name, sig);
            }
        }

        for (const auto& astFunc : functions.functions) {
            if (!astFunc->name) {
                continue;
            }
            try {
                auto sig = signatures.at(astFunc->name->identifier.name.name);
                auto cfgFuncEm = jac::cfg::emit(*astFunc, sig, signatures);
                auto cfgFunc = cfgFuncEm.output();
                result.emplace_back(std::move(cfgFunc), astFunc->code);
            }
            catch (const std::exception& e) {
                std::cerr << "Error compiling function " << astFunc->name->identifier.name.name << ": " << e.what() << '\n';
                signatures.erase(astFunc->name->identifier.name.name);
                continue;
            }
        }

        // remove functions that depend on non-simple functions
        bool erased = true;
        while (erased) {
            erased = false;
            auto it = result.begin();
            while (it != result.end()) {
                auto& cfgFunc = it->first;
                for (const auto& req : cfgFunc.requiredFunctions) {
                    if (!signatures.contains(req)) {
                        signatures.erase(cfgFunc.name());
                        result.erase(it++);
                        erased = true;
                        goto next;
                    }
                }
                ++it;
                next:;
            }
        }

        return result;
    }

    std::string placeStubs(std::string_view js, const std::list<CompFn*>& compiledFunctions) {
        std::stringstream newJs;
        auto begin = js.begin();
        for (const auto& fn : compiledFunctions) {
            newJs << std::string_view(begin, fn->code.begin());

            newJs << "const ";
            newJs << fn->name;
            newJs << " = ";
            newJs << fn->alias;
            newJs << "; /* compiled from: ";
            newJs << fn->code;
            newJs << " */";

            begin = fn->code.end();
        }

        newJs << std::string_view(begin, js.end());

        return newJs.str();
    }

    std::list<CompFn*> compile(std::list<std::pair<jac::cfg::Function, std::string_view>> cfgFunctions) {
        std::list<CompFn*> results;

        cwerg::base::Unit unit = cwerg::base::UnitNew(cwerg::base::StrNew("jit"));
        for (auto& [cfgFunc, code] : cfgFunctions) {
            auto res = jac::cfg::compile(unit, cfgFunc);
            cwerg::base::FunRenderToAsm(res, &std::cout);
        }

        cwerg::code_gen_x64::LegalizeAll(unit, false, nullptr);
        cwerg::code_gen_x64::RegAllocGlobal(unit, false, nullptr);
        cwerg::code_gen_x64::RegAllocLocal(unit, false, nullptr);

        cwerg::x64::X64Unit x64unit = cwerg::code_gen_x64::EmitUnitAsBinary(unit);
        auto textMemory = static_cast<uint32_t*>(mmap(nullptr, 4096, PROT_READ | PROT_WRITE | PROT_EXEC,
                                                      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));

        x64unit.sec_text->shdr.sh_addr = reinterpret_cast<uint64_t>(textMemory);  // NOLINT
        memcpy(textMemory, x64unit.sec_text->data->data(), x64unit.sec_text->data->size());

        for (auto& sym : x64unit.symbols) {
            ASSERT(sym->sym.st_value != ~0U, "undefined symbol " << sym->name);
            if (sym->section != nullptr) {
                ASSERT(sym->section->shdr.sh_addr != ~0U,
                            sym->name << "has bad sec " << *sym->section);
                sym->sym.st_value += sym->section->shdr.sh_addr;
                std::cerr << "sym [" << sym->name << "]: 0x" <<   sym->sym.st_value << "\n";
            }
        }
        for (auto& rel : x64unit.relocations) {
            cwerg::x64::ApplyRelocation(rel);
        }

        for (auto& [cfgFunc, code] : cfgFunctions) {
            auto aliasName = alias(cfgFunc.entry);
            auto name = cfgFunc.name();
            auto compFn = CompFn{ name, aliasName, code, {} };

            auto [it, _] = compiledHolder.emplace(name, std::move(compFn));

            // TODO: add caller converting args from array
            auto jsFn = detail::CompiledCallable<int32_t>{ ??? };  // NOLINT

            FunctionFactory ff(this->context());
            Function jsFnObj = ff.newFunctionThisVariadic(jsFn);

            it->second.jsFn = jsFnObj;

            results.push_back(&it->second);
        }

        return results;
    }

    std::string tryAot(std::string_view js, [[maybe_unused]] std::string_view filename, [[maybe_unused]] EvalFlags flags) {
        jac::ast::Script script = parseScript(js);

        jac::ast::traverse::Functions astFunctions;
        jac::ast::traverse::funcs(script, astFunctions);

        auto simple = transformFunctions(astFunctions);

        auto compiledFunctions = compile(std::move(simple));

        compiledFunctions.sort([](const CompFn* a, const CompFn* b) {
            return a->code.begin() < b->code.begin();
        });

        auto newJS = placeStubs(js, compiledFunctions);

        // define the compiled functions in global scope
        jac::Object global = this->context().getGlobalObject();

        for (auto& fn : compiledFunctions) {
            assert(fn->jsFn);
            global.defineProperty(fn->alias, *(fn->jsFn));
        }

        return newJS;
    }
public:
    AotEvalFeature(): EvalFeature<Next>() {
        static bool initialized = false;
        if (!initialized) {
            cwerg::InitStripes(10);  // FIXME: reset, reinitialize
            cwerg::code_gen_x64::InitCodeGenX64();
            initialized = true;
        }
    }

    /**
     * @brief Evaluate a string containing javascript code, while compiling some
     * parts to native code
     *
     * @param code the code to evaluate
     * @param filename filename to use for the code. Used for error reporting
     * @param flags flags to evaluate the code with
     * @return Result of the evaluation
     */
    Value eval(std::string code, std::string filename, EvalFlags flags = EvalFlags::Global) {
        try {
            code = tryAot(code, filename, flags);
            std::cout << code << '\n';
        }
        catch (std::exception& e) {
            std::cerr << "Error during AOT compilation: " << e.what() << '\n';
            return EvalFeature<Next>::eval(std::move(code), filename, flags);
        }

        return EvalFeature<Next>::eval(std::move(code), filename, flags);
    }
};


} // namespace jac
