#pragma once

#include <jac/machine/machine.h>


namespace jac {


template<class Next>
class EvalFeature : public Next {
public:
    /**
     * @brief Evaluate a string containing javascript code
     *
     * @param code the code to evaluate
     * @param filename filename to use for the code. Used for error reporting
     * @param flags flags to evaluate the code with
     * @return Result of the evaluation
     */
    Value eval(std::string code, std::string filename, EvalFlags flags = EvalFlags::Global) {
        this->resetWatchdog();
        Value bytecode(this->context(), JS_Eval(this->context(), code.c_str(), code.size(), filename.c_str(), static_cast<int>(flags | EvalFlags::CompileOnly)));
        if (static_cast<int>(flags & EvalFlags::CompileOnly) != 0) {
            return bytecode;
        }
        code = "";
        this->resetWatchdog();
        return Value(this->context(), JS_EvalFunction(this->context(), bytecode.loot().second));
    }
};


} // namespace jac
