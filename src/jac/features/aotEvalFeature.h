#pragma once

#include <jac/features/evalFeature.h>
#include <jac/machine/functionFactory.h>
#include <jac/machine/machine.h>

#include <jac/machine/compiler/ast.h>
#include <jac/machine/compiler/scanner.h>
#include <jac/machine/compiler/tacEmit.h>
#include <jac/machine/compiler/tacMir.h>
#include <jac/machine/compiler/tacTree.h>
#include <jac/machine/compiler/traverseFuncs.h>

#include <list>
#include <mir-gen.h>
#include <mir.h>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>


namespace jac {


namespace detail {

inline MIR_item_t MIR_get_global_item(MIR_context_t ctx, std::string_view name) {
    for (MIR_module_t module = DLIST_HEAD(MIR_module_t, *MIR_get_module_list(ctx));
         module != nullptr;
         module = DLIST_NEXT(MIR_module_t, module)
    ) {
        for (MIR_item_t item = DLIST_HEAD(MIR_item_t, module->items);
             item != nullptr;
             item = DLIST_NEXT(MIR_item_t, item)
        ) {
            if (name == MIR_item_name(ctx, item)) {
                return item;
            }
        }
    }
    return nullptr;
}

template<typename Res>
struct wrapper {
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
    struct FunctionInfo {
        std::string alias;
        Function fn;
    };

    struct CompFn {
        FunctionInfo* fn;
        std::string name;
        std::string_view code;
    };

    std::vector<MIR_context_t> mirContexts;

    std::map<std::string, FunctionInfo> compiledHolder;

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

    std::list<std::pair<jac::tac::Function, std::string_view>> transformFunctions(const jac::ast::traverse::Functions& functions) {
        std::list<std::pair<jac::tac::Function, std::string_view>> result;

        std::set<std::string> simpleFunctions;

        for (const auto& astFunc : functions.functions) {
            try {
                result.emplace_back(jac::tac::emit(*astFunc), astFunc->code);
                simpleFunctions.insert(result.back().first.name());
            }
            catch (const std::exception& e) {
                std::cerr << "Error compiling function " << astFunc->name->identifier.name.name << ": " << e.what() << '\n';
                continue;
            }
        }

        // remove functions that depend on non-simple functions
        bool erased = true;
        while (erased) {
            erased = false;
            auto it = result.begin();
            while (it != result.end()) {
                auto& tacFunc = it->first;
                for (const auto& req : tacFunc.requiredFunctions()) {
                    if (!simpleFunctions.contains(req)) {
                        simpleFunctions.erase(tacFunc.name());
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

    std::string getMir(const std::list<std::pair<jac::tac::Function, std::string_view>>& tacFunctions) {
        std::ostringstream mir;
        mir << "mod: module\n";

        for (const auto& tacFunc : tacFunctions) {
            jac::tac::mir::generateProto(mir, tacFunc.first);
        }

        for (const auto& tacFunc : tacFunctions) {
            jac::tac::mir::generate(mir, tacFunc.first);
            jac::tac::mir::generateCaller(mir, tacFunc.first);
        }

        mir << "endmodule\n";

        return mir.str();
    }

    auto compileMir(std::string& mir, const std::list<std::pair<jac::tac::Function, std::string_view>>& tacFunctions) {
        std::vector<CompFn> compiledFunctions;

        MIR_context_t mirCtx = MIR_init();
        mirContexts.push_back(mirCtx);

        MIR_scan_string(mirCtx, mir.c_str());

        MIR_module_t mod = DLIST_HEAD(MIR_module_t, *MIR_get_module_list(mirCtx));
        MIR_load_module(mirCtx, mod);

        MIR_gen_init(mirCtx, 1);
        MIR_link(mirCtx, MIR_set_gen_interface, nullptr);

        for (auto& tacFunc : tacFunctions) {
            std::string name = "f_" + tacFunc.first.name();
            MIR_item_t func = detail::MIR_get_global_item(mirCtx, name);
            if (!func) {
                throw std::runtime_error("Function not found " + name);
            }

            MIR_item_t caller = detail::MIR_get_global_item(mirCtx, "caller_" + tacFunc.first.name());
            if (!caller) {
                throw std::runtime_error("Caller not found");
            }

            void* ptr = MIR_gen(mirCtx, 0, func);
            void* callerPtr = MIR_gen(mirCtx, 0, caller);

            FunctionFactory ff(this->context());

            Function jsFn = [&]() {
                switch (tacFunc.first.returnType()) {
                    case jac::tac::ValueType::I32:
                        return ff.newFunctionVariadic(detail::wrapper<int32_t>{callerPtr});
                    case jac::tac::ValueType::Double:
                        return ff.newFunctionVariadic(detail::wrapper<double>{callerPtr});
                    case jac::tac::ValueType::Bool:
                        return ff.newFunctionVariadic(detail::wrapper<bool>{callerPtr});
                    case jac::tac::ValueType::Void:
                        return ff.newFunctionVariadic(detail::wrapper<void>{callerPtr});
                    default:
                        throw std::runtime_error("Invalid return type");
                }
            }();

            auto [it, was] = compiledHolder.emplace(name, FunctionInfo{ alias(ptr), jsFn });
            compiledFunctions.push_back({ &it->second, tacFunc.first.name(), tacFunc.second });
        }

        MIR_gen_finish(mirCtx);

        return compiledFunctions;
    }

    std::string placeStubs(std::string_view js, const std::vector<CompFn>& compiledFunctions) {
        std::stringstream newJs;
        auto begin = js.begin();
        for (const auto& compFn : compiledFunctions) {

            newJs << std::string_view(begin, compFn.code.begin());

            newJs << "var ";
            newJs << compFn.name;
            newJs << " = ";
            newJs << compFn.fn->alias;
            newJs << "; /* compiled from: ";
            newJs << compFn.code;
            newJs << " */";

            begin = compFn.code.end();
        }

        newJs << std::string_view(begin, js.end());

        return newJs.str();
    }

    std::string tryAot(std::string_view js, std::string_view filename, EvalFlags flags) {
        jac::ast::Script script = parseScript(js);

        jac::ast::traverse::Functions astFunctions;
        jac::ast::traverse::funcs(script, astFunctions);

        std::list<std::pair<jac::tac::Function, std::string_view>> simple = transformFunctions(astFunctions);

        auto mirStr = getMir(simple);
        auto compiledFunctions = compileMir(mirStr, simple);

        std::sort(compiledFunctions.begin(), compiledFunctions.end(), [](const CompFn& a, const CompFn& b) {
            return a.code.begin() < b.code.begin();
        });

        return placeStubs(js, compiledFunctions);
    }
public:

    ~AotEvalFeature() {
        for (auto& ctx : mirContexts) {
            MIR_finish(ctx);
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
        }
        catch (std::exception& e) {
            std::cerr << "Error during AOT compilation: " << e.what() << '\n';
            return EvalFeature<Next>::eval(std::move(code), filename, flags);
        }

        // define the compiled functions in global scope
        jac::Object global = this->context().getGlobalObject();

        for (auto& [ _, fn ] : compiledHolder) {
            global.defineProperty(fn.alias, fn.fn);
        }

        return EvalFeature<Next>::eval(std::move(code), filename, flags);
    }
};


} // namespace jac
