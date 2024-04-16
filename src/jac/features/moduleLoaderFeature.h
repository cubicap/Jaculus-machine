#pragma once

#include <jac/machine/machine.h>


namespace jac {


template<class Next>
class ModuleLoaderFeature : public Next {
private:
    static JSModuleDef *moduleLoaderCbk(JSContext* ctx, const char *module_name, void *_self) {
        auto &self = *static_cast<ModuleLoaderFeature<Next>*>(_self);

        std::string filename = module_name;

        std::string buffer;
        try {
            buffer = self.fs.loadCode(filename);
        } catch (jac::Exception &e) {
            e.throwJS(ctx);
            return nullptr;
        }

        // compile and return module
        self.resetWatchdog();

        JSValue val;
        try {
            Value code = self.eval(std::move(buffer), std::move(filename), EvalFlags::Module | EvalFlags::CompileOnly);
            val = code.loot().second;
        }
        catch (...) {
            return nullptr;
        }

        auto mdl = static_cast<JSModuleDef*>(JS_VALUE_GET_PTR(val));

        Object meta(ctx, JS_GetImportMeta(ctx, mdl));
        meta.set("url", filename);
        meta.set("main", false);

        return mdl;
    }

public:
    Value evalFile(std::string path_) {
        auto buffer = this->fs.loadCode(path_);

        Value val = this->eval(std::move(buffer), path_, EvalFlags::Module);
        return val;
    }

    void initialize() {
        Next::initialize();

        JS_SetModuleLoaderFunc(this->runtime(), nullptr, moduleLoaderCbk, this);
    }
};


} // namespace jac
