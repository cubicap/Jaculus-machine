#include "machine.h"
#include "functionFactory.h"


namespace jac {


Module::Module(ContextRef ctx, std::string name) : _ctx(ctx) {
    _def = JS_NewCModule(ctx, name.c_str(), [](JSContext* context, JSModuleDef* def) {
        Module& module = base(context).findModule(def);

        for (auto& [exName, exVal] : module.exports) {
            JS_SetModuleExport(context, def, exName.c_str(), exVal.loot().second);
        }
        return 0;
    });
    if (!_def) {
        throw std::runtime_error("JS_NewCModule failed");
    }
}

void Module::addExport(std::string name, Value val) {
    JS_AddModuleExport(_ctx, _def, name.c_str());
    exports.emplace_back(name, val);
}

void MachineBase::initialize() {
    // last in stack

    _runtime = JS_NewRuntime();
    _context = JS_NewContext(_runtime);

    JS_SetContextOpaque(_context, this);
    JS_SetInterruptHandler(_runtime, [](JSRuntime* rt, void* opaque) noexcept {
        MachineBase& base = *reinterpret_cast<MachineBase*>(opaque);
        if (base._interrupt) {
            base._interrupt = false;
            return 1;
        }
        if (base._watchdogTimeout.count() > 0) {
            auto now = std::chrono::steady_clock::now();
            if (now > base._watchdogNext) {
                base._watchdogNext = now + base._watchdogTimeout;
                if (!base._wathdogCallback || base._wathdogCallback()) {
                    return 1;
                }
            }
        }
        return 0;
    }, this);
}

Value MachineBase::eval(std::string code, std::string filename, EvalFlags flags /*= EvalFlags::Global*/) {
    Value bytecode(_context, JS_Eval(_context, code.c_str(), code.size(), filename.c_str(), static_cast<int>(flags | EvalFlags::CompileOnly)));
    if (static_cast<int>(flags & EvalFlags::CompileOnly) != 0) {
        return bytecode;
    }
    code = "";
    resetWatchdog();
    return Value(_context, JS_EvalFunction(_context, bytecode.loot().second));
}

void MachineBase::registerGlobal(std::string name, Value value, PropFlags flags /*= PropFlags::Enumerable*/) {
    Object globalObject(_context, JS_GetGlobalObject(_context));
    globalObject.defineProperty(name, value, flags);
}

Module& MachineBase::newModule(std::string name) {
    Module module(_context, name);
    JSModuleDef* def = module.get();
    _modules.emplace(def, std::move(module));

    return _modules.find(def)->second;
}

Module& MachineBase::findModule(JSModuleDef* m) {
    auto it = _modules.find(m);
    if (it == _modules.end()) {
        throw std::runtime_error("module not found");
    }
    return it->second;
}


} // namespace jac
