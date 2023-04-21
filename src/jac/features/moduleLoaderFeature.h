#pragma once

#include <jac/machine/machine.h>


template<class Next>
class ModuleLoaderFeature : public Next {
private:
    static JSModuleDef *moduleLoaderCbk(JSContext* ctx, const char *module_name, void *_self) {
        auto &self = *reinterpret_cast<ModuleLoaderFeature<Next>*>(_self);

        std::string filename = module_name;

        std::string buffer;
        try {
            buffer = self.fs.loadCode(filename);
        } catch (std::exception &e) {
            auto error = jac::Exception::create(jac::Exception::Type::Error, e.what());
            error.throwJS(ctx);
            return nullptr;
        }

        // compile and return module
        self.resetWatchdog();
        JSValue val = JS_Eval(ctx, buffer.c_str(), buffer.size(), module_name,
                              JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY);
        if (JS_IsException(val)) {
            return nullptr;
        }

        JSModuleDef *mdl = reinterpret_cast<JSModuleDef*>(JS_VALUE_GET_PTR(val));

        jac::Object meta(ctx, JS_GetImportMeta(ctx, mdl));
        meta.set("url", filename);
        meta.set("main", false);

        return mdl;
    }

public:
    jac::Value evalFile(std::string path_) {
        auto buffer = this->fs.loadCode(path_);

        jac::Value val = this->eval(std::move(buffer), path_, jac::EvalFlags::Module);
        return val;
    }

    void initialize() {
        Next::initialize();

        JS_SetModuleLoaderFunc(this->runtime(), nullptr, moduleLoaderCbk, this);
    }
};
