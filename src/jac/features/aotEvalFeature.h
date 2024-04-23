#pragma once

#include "jac/machine/internal/declarations.h"
#include <jac/features/evalFeature.h>
#include <jac/machine/functionFactory.h>
#include <jac/machine/machine.h>

#include <jac/machine/compiler/ast.h>
#include <jac/machine/compiler/scanner.h>
#include <jac/machine/compiler/tacEmit.h>
#include <jac/machine/compiler/tacMir.h>
#include <jac/machine/compiler/tacTree.h>
#include <jac/machine/compiler/traverseFuncs.h>

#include <mir-gen.h>
#include <mir.h>

#include <algorithm>
#include <cstdint>
#include <iostream>
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

} // namespace detail


template<class Next>
class AotEvalFeature : public EvalFeature<Next> {
    MIR_context_t mirCtx;

    std::vector<std::pair<std::string, Function>> compiledHolder;

    template<bool Yield, bool Await, bool Default>
    std::pair<Function, void*> compileFunction(const jac::ast::FunctionDeclaration<Yield, Await, Default>& astFunc) {
        jac::tac::Function tacFunc = jac::tac::emit(astFunc);

        std::stringstream ss;
        ss << "mod: module\n";

        // --- function ---
        jac::tac::mir::generate(ss, tacFunc);

        // prototype
        tac::mir::generateProto(ss, tacFunc);

        // --- caller ---
        ss << "caller: func i64, i64:argc, p:argv\n";


        ss << "    local i64:pos, i64:res";
        for (size_t i = 0; i < tacFunc.args.size(); ++i) {
            ss << ", i64:arg" << i;
        }
        ss << '\n';
        ss << "body:\n";

        // check number of arguments
        ss << "    bgt fail, argc, " << tacFunc.args.size() << '\n';

        // prepare arguments
        for (size_t i = 0; i < tacFunc.args.size(); ++i) {
            const auto& [arg, type] = tacFunc.args[i];
            ss << "    mov pos, " << i * 2 << "\n";
            switch (type) {
                case tac::ValueType::I8:
                    ss << "    mov arg" << i << ", i8:(argv, pos, 8)\n";
                    break;
                case tac::ValueType::U8:
                    ss << "    mov arg" << i << ", u8:(argv, pos, 8)\n";
                    break;
                case tac::ValueType::I16:
                    ss << "    mov arg" << i << ", i16:(argv, pos, 8)\n";
                    break;
                case tac::ValueType::U16:
                    ss << "    mov arg" << i << ", u16:(argv, pos, 8)\n";
                    break;
                case tac::ValueType::I32:
                    ss << "    mov arg" << i << ", i32:(argv, pos, 8)\n";
                    break;
                case tac::ValueType::U32:
                    ss << "    mov arg" << i << ", u32:(argv, pos, 8)\n";
                    break;
                default:
                    throw std::runtime_error("Unsupported argument type" + std::to_string(static_cast<int>(type)));
            }
        }

        // call
        ss << "    call p_" << tacFunc.name << ", " << tacFunc.name << ", res";
        for (size_t i = 0; i < tacFunc.args.size(); ++i) {
            ss << ", arg" << i;
        }
        ss << '\n';
        ss << "    ret res\n";

        ss << "fail:\n";
        // TODO: throw exception
        ss << "    ret 0\n";
        ss << "endfunc\n";
        ss << "endmodule\n";

        auto code = ss.str();


        MIR_scan_string(mirCtx, code.c_str());

        MIR_module_t mod = DLIST_HEAD(MIR_module_t, *MIR_get_module_list (mirCtx));
        MIR_load_module(mirCtx, mod);
        MIR_link(mirCtx, MIR_set_interp_interface, nullptr);

        std::string name = jac::tac::id(astFunc.name->identifier.name.name);
        MIR_item_t func = detail::MIR_get_global_item(mirCtx, name);
        if (!func) {
            throw std::runtime_error("Function not found");
        }

        MIR_item_t caller = detail::MIR_get_global_item(mirCtx, "caller");
        if (!caller) {
            throw std::runtime_error("Caller not found");
        }

        MIR_gen_init(mirCtx, 1);
        void* ptr = MIR_gen(mirCtx, 0, func);
        void* callerPtr = MIR_gen(mirCtx, 0, caller);

        MIR_gen_finish(mirCtx);

        FunctionFactory ff(this->context());
        auto jsFn = ff.newFunctionVariadic([callerPtr](std::vector<ValueWeak> args) {
            std::vector<JSValue> jsArgs;
            for (ValueWeak& arg : args) {
                jsArgs.push_back(arg.getVal());
            }

            auto fn = reinterpret_cast<int64_t(*)(int64_t, JSValue*)>(callerPtr); // NOLINT

            return static_cast<int32_t>(fn(jsArgs.size(), jsArgs.data()));
        });


        return { jsFn, ptr };
    }

    std::string tryAot(std::string_view js, std::string_view filename, EvalFlags flags) {
        bool hadError = false;
        jac::lex::Scanner scanner(js, [&hadError](int line, int col, const std::string& msg) {
            hadError = true;
        });

        if (hadError) {
            throw std::runtime_error("Lex error");
        }

        auto tokens = scanner.scan();

        jac::ast::ParserState state(tokens);

        auto script = jac::ast::Script::parse(state);
        if (!script) {
            throw std::runtime_error("Parse error");
        }

        jac::ast::traverse::Functions functions;

        jac::ast::traverse::funcs(*script, functions);

        struct CompFn {
            Function fn;
            std::string_view name;
            std::string_view code;
            void* ptr;
        };

        std::vector<CompFn> compiledFunctions;

        for (auto func : functions.functions) {
            try {
                auto [ fn, ptr ] = compileFunction(*func);
                compiledFunctions.push_back({ fn, func->name->identifier.name.name, func->code, ptr });
            }
            catch (std::runtime_error& e) {
                throw std::runtime_error("Failed to compile function: " + std::string(e.what()));
            }
        }

        std::sort(compiledFunctions.begin(), compiledFunctions.end(), [](const CompFn& a, const CompFn& b) {
            return a.code.begin() < b.code.begin();
        });

        std::string newCode;
        auto begin = js.begin();
        for (auto& compFn : compiledFunctions) {
            std::string alias = "__jac_aot_func_" + std::to_string(reinterpret_cast<uint64_t>(compFn.ptr));  // NOLINT

            newCode.append(begin, compFn.code.begin());

            newCode.append("var ");
            newCode.append(compFn.name);
            newCode.append(" = ");
            newCode.append(alias);
            newCode.append("; /* compiled from: ");
            newCode.append(compFn.code);
            newCode.append(" */");

            begin = compFn.code.end();

            compiledHolder.push_back({ alias, compFn.fn });
        }

        newCode.append(begin, js.end());

        return newCode;
    }
public:
    AotEvalFeature() : mirCtx(MIR_init()) {
    }

    ~AotEvalFeature() {
        MIR_finish(mirCtx);
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
            std::string newCode = tryAot(code, filename, flags);

            // define the compiled functions in global scope
            jac::Object global = this->context().getGlobalObject();

            for (auto& [ name, fn ] : compiledHolder) {
                global.defineProperty(name, fn);
            }

            return EvalFeature<Next>::eval(std::move(newCode), filename, flags);
        }
        catch (...) {
            return EvalFeature<Next>::eval(code, filename, flags);
        }
    }
};


} // namespace jac