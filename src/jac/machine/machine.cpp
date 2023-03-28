#include "machine.h"
#include "functionFactory.h"

namespace jac {


JSModule::JSModule(ContextRef ctx, std::string name) : _ctx(ctx) {
    _def = JS_NewCModule(ctx, name.c_str(), [](JSContext* context, JSModuleDef* def) {
        JSModule& module = base(context).findModule(def);

        for (auto& [exName, exVal] : module.exports) {
            JS_SetModuleExport(context, def, exName.c_str(), exVal.loot().second);
        }
        return 0;
    });
    if (!_def) {
        throw std::runtime_error("JS_NewCModule failed");
    }
}

void JSModule::addExport(std::string name, Value val) {
    JS_AddModuleExport(_ctx, _def, name.c_str());
    exports.emplace_back(name, val);
}

void MachineBase::initialize() {
    // last in stack

    _runtime = JS_NewRuntime();
    _context = JS_NewContext(_runtime);

    JS_SetContextOpaque(_context, this);
    JS_SetInterruptHandler(_runtime, [](JSRuntime* rt, void* opaque) {
        MachineBase& base = *reinterpret_cast<MachineBase*>(opaque);
        if (base._interrupt) {
            base._interrupt = false;
            return 1;
        }
        return 0;
    }, this);
}

Value MachineBase::eval(std::string code, std::string filename, int eval_flags) {
    return Value(_context, JS_Eval(_context, code.c_str(), code.size(), filename.c_str(), eval_flags));
}

void MachineBase::registerGlobal(std::string name, Value value) {
    Object globalObject(_context, JS_GetGlobalObject(_context));
    globalObject.set(name, value);
}

JSModule& MachineBase::newModule(std::string name) {
    JSModule module(_context, name);
    JSModuleDef* def = module.get();
    _modules.emplace(def, std::move(module));

    return _modules.find(def)->second;
}

JSModule& MachineBase::findModule(JSModuleDef* m) {
    auto it = _modules.find(m);
    if (it == _modules.end()) {
        throw std::runtime_error("module not found");
    }
    return it->second;
}


} // namespace jac
