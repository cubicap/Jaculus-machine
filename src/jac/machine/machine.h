#pragma once

#include <quickjs.h>

#include <string>
#include <stdexcept>
#include <functional>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <string.h>
#include "values.h"


namespace jac {


enum class EvalFlags : int {
    Global = JS_EVAL_TYPE_GLOBAL,
    Module = JS_EVAL_TYPE_MODULE,

    Strict = JS_EVAL_FLAG_STRICT,
    Strip = JS_EVAL_FLAG_STRIP,
    CompileOnly = JS_EVAL_FLAG_COMPILE_ONLY,
    BacktraceBarrier = JS_EVAL_FLAG_BACKTRACE_BARRIER
};

inline constexpr EvalFlags operator|(EvalFlags a, EvalFlags b) {
    int res = static_cast<int>(a) | static_cast<int>(b);
    if (res & JS_EVAL_TYPE_GLOBAL && res & JS_EVAL_TYPE_MODULE) {
        throw std::runtime_error("Cannot use both global and module eval flags");
    }
    return static_cast<EvalFlags>(res);
}

inline constexpr EvalFlags operator&(EvalFlags a, EvalFlags b) {
    return static_cast<EvalFlags>(static_cast<int>(a) & static_cast<int>(b));
}


class MachineBase;


/**
 * @brief A wrapper around JSModuleDef that allows for easy exporting of values
 */
class Module {
    ContextRef _ctx;
    JSModuleDef *_def;

    std::vector<std::tuple<std::string, Value>> exports;

    inline MachineBase& base() {
        return *reinterpret_cast<MachineBase*>(JS_GetContextOpaque(_ctx));
    }

    static inline MachineBase& base(ContextRef ctx) {
        return *reinterpret_cast<MachineBase*>(JS_GetContextOpaque(ctx));
    }
public:
    /**
     * @brief Create a new module in the given context. Should not be called
     * directly, use MachineBase::newModule instead
     *
     * @param ctx context to work in
     * @param name name of the module
     */
    Module(ContextRef ctx, std::string name);
    Module& operator=(const Module&) = delete;
    Module(const Module&) = delete;
    Module& operator=(Module&& other) {
        _ctx = other._ctx;
        _def = other._def;
        exports = std::move(other.exports);
        other._def = nullptr;
        return *this;
    }
    Module(Module&& other): _ctx(other._ctx), _def(other._def), exports(std::move(other.exports)) {
        other._def = nullptr;
    }

    /**
     * @brief Add a value to the module's exports
     *
     * @param name name of the export
     * @param val the value to export
     */
    void addExport(std::string name, Value val);

    /**
     * @brief Get the underlying JSModuleDef* for this module
     *
     * @return JSModuleDef*
     */
    JSModuleDef *get() {
        return _def;
    }
};


class MachineBase {
private:
    std::unordered_map<JSModuleDef*, Module> _modules;
    Module& findModule(JSModuleDef* m);

    bool _interrupt = false;
public:
    JSRuntime* _runtime = nullptr;
    ContextRef _context = nullptr;

    /**
     * @brief Initialize the machine. Should be called after machine configuration
     * is done and before any interaction with the javascript engine.
     * Other features in the Machine stack can implement this method to
     * initialize themselves but the first call should be Next::initialize().
     */
    void initialize();

    MachineBase() = default;
    MachineBase(const MachineBase&) = delete;
    MachineBase(MachineBase&&) = delete;
    MachineBase& operator=(const MachineBase&) = delete;
    MachineBase& operator=(MachineBase&&) = delete;

    virtual ~MachineBase() {
        _modules.clear();
        if (_context) {
            _context.free();
        }
        if (_runtime) {
            JS_FreeRuntime(_runtime);
        }
    }

    /**
     * @brief Evaluate a string containing javascript code
     *
     * @param code the code to evaluate
     * @param filename filename to use for the code. Used for error reporting
     * @param flags flags to evaluate the code with
     * @return Result of the evaluation
     */
    Value eval(std::string code, std::string filename, EvalFlags flags = EvalFlags::Global);

    /**
     * @brief Register a property on the global object
     *
     * @param name name of the property
     * @param value value of the property
     * @param flags flags of the property
     */
    void registerGlobal(std::string name, Value value, PropFlags flags = PropFlags::Enumerable);

    /**
     * @brief Create a new module in the machine
     *
     * @param name name of the module
     * @return Reference to the new module
     */
    Module& newModule(std::string name);

    /**
     * @brief Interrupt running javascript code. Execution will be thrown
     * in the javascript as an InterruptError.
     */
    void interruptRuntime() {
        _interrupt = true;
    }

    friend class Module;
};


} // namespace jac
