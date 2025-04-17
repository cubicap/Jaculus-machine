#pragma once

#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <map>
#include <memory>
#include <type_traits>
#include <unordered_map>
#include <utility>

#include <jac/machine/compiler/cfg.h>
#include <jac/util.h>
#include <quickjs.h>

#include "types.h"


namespace jac::cfg::interp {


template<typename Id>
struct RefCounter {
    struct ScreamingCount {
        int value = 1;

        void increment() {
            assert(value > 0 && "Reference count already zero");
            value++;
        }

        void decrement() {
            assert(value > 0 && "Reference count already zero");
            value--;
        }

        int32_t get() const {
            return value;
        }

        ~ScreamingCount() {
            assert(value == 0 && "Reference count not zero");
        }
    };

    std::shared_ptr<ScreamingCount> count;

    RefCounter() = delete;
    RefCounter(std::shared_ptr<ScreamingCount> c) : count(c) {}
    static RefCounter create() {
        return RefCounter(std::make_shared<ScreamingCount>());
    }

    void increment() { count->increment(); }
    void decrement() { count->decrement(); }
};


struct Int32 {
    int32_t value;
    RefCounter<Int32> refCounter;

    Int32(int32_t v) : value(v), refCounter(RefCounter<Int32>::create()) {}

    int32_t& operator*() {
        return value;
    }

    void _dup(JSContext*) {
        refCounter.increment();
    }

    void _free(JSContext*) {
        refCounter.decrement();
    }
};

struct Float {
    double value;
    RefCounter<Float> refCounter;

    Float(double v) : value(v), refCounter(RefCounter<Float>::create()) {}

    double& operator*() {
        return value;
    }

    void _dup(JSContext*) {
        refCounter.increment();
    }

    void _free(JSContext*) {
        refCounter.decrement();
    }
};


struct Bool {
    bool value;
    RefCounter<Bool> refCounter;

    Bool(bool v) : value(v), refCounter(RefCounter<Bool>::create()) {}

    bool& operator*() {
        return value;
    }

    void _dup(JSContext*) {
        refCounter.increment();
    }

    void _free(JSContext*) {
        refCounter.decrement();
    }
};

struct ObjectPtr {
    JSObject* value;

    ObjectPtr(JSObject* v) : value(v) {}

    JSObject*& operator*() {
        return value;
    }

    void _dup(JSContext* ctx) {
        auto obj = JS_MKPTR(JS_TAG_OBJECT, value);
        JS_DupValue(ctx, obj);
    }

    void _free(JSContext* ctx) {
        auto val = JS_MKPTR(JS_TAG_OBJECT, value);
        JS_FreeValue(ctx, val);
    }
};

struct StringConst {
    std::string const* value;

    StringConst(const std::string& v) : value(&v) {}

    const std::string& operator*() {
        return *value;
    }

    void _dup(JSContext*) {
        // no op
    }

    void _free(JSContext*) {
        // no op
    }
};

struct String {
    char* value;

    char*& operator*() {
        return value;
    }

    void _dup(JSContext* ctx) {
        if (value) {
            JS_DupValue(ctx, JS_MKPTR(JS_TAG_STRING, value));
        }
    }

    void _free(JSContext* ctx) {
        if (value) {
            JS_FreeCString(ctx, value);
        }
    }
};

struct Any {
    JSValue value;

    Any(JSValue v) : value(v) {}

    JSValue& operator*() {
        return value;
    }

    void _dup(JSContext* ctx) {
        JS_DupValue(ctx, value);
    }

    void _free(JSContext* ctx) {
        JS_FreeValue(ctx, value);
    }
};


template<typename T>
concept I32Able = std::is_same_v<T, Int32> || std::is_same_v<T, Bool>;

template<typename L>
concept DoubleAble = std::is_same_v<L, Float> || I32Able<L>;

template<typename T>
struct ConvertVisitor {};

template<>
struct ConvertVisitor<Int32> {
    JSContext* ctx;

    Int32 operator()(Int32 v) {
        return v;
    }
    Int32 operator()(DoubleAble auto v) {
        return static_cast<int32_t>(*v);
    }
    Int32 operator()(Any v) {
        int32_t res;
        if (JS_ToInt32(ctx, &res, *v) < 0) {
            throw std::runtime_error("Invalid conversion to Int32");
        }
        return res;
    }
    Int32 operator()(auto) {
        throw std::runtime_error("Invalid conversion to Int32");
    }
};

template<>
struct ConvertVisitor<Float> {
    JSContext* ctx;

    Float operator()(Float v) {
        return v;
    }
    Float operator()(DoubleAble auto v) {
        return static_cast<double>(*v);
    }
    Float operator()(Any v) {
        double res;
        if (JS_ToFloat64(ctx, &res, *v) < 0) {
            throw std::runtime_error("Invalid conversion to Float");
        }
        return res;
    }
    Float operator()(auto) {
        throw std::runtime_error("Invalid conversion to Double");
    }
};

template<>
struct ConvertVisitor<Bool> {
    JSContext* ctx;

    Bool operator()(Bool v) {
        return v;
    }
    Bool operator()(DoubleAble auto v) {
        return *v != 0;
    }
    Bool operator()(Any v) {
        int32_t res = JS_ToBool(ctx, *v);
        if (res < 0) {
            throw std::runtime_error("Invalid conversion to Bool");
        }
        return res != 0;
    }
    Bool operator()(auto) {
        throw std::runtime_error("Invalid conversion to Bool");
    }
};

template<>
struct ConvertVisitor<ObjectPtr> {
    JSContext* ctx;

    ObjectPtr operator()(ObjectPtr v) {
        return v;
    }
    ObjectPtr operator()(Any v) {
        if (JS_VALUE_GET_TAG(*v) == JS_TAG_OBJECT) {
            JS_DupValue(ctx, *v);
            return JS_VALUE_GET_OBJ(*v);
        }
        throw std::runtime_error("Invalid conversion to ObjectPtr");
    }
    ObjectPtr operator()(auto) {
        throw std::runtime_error("Invalid conversion to Object");
    }
};

template<>
struct ConvertVisitor<StringConst> {
    JSContext* ctx;

    StringConst operator()(StringConst v) {
        return v;
    }
    StringConst operator()(auto) {
        throw std::runtime_error("Invalid conversion to StringConst");
    }
};

template<>
struct ConvertVisitor<Any> {
    JSContext* ctx;

    Any operator()(Any v) {
        return v;
    }
    Any operator()(ObjectPtr v) {
        JSValue val = JS_MKPTR(JS_TAG_OBJECT, *v);
        JS_DupValue(ctx, val);
        return val;
    }
    Any operator()(Int32 v) {
        return JS_NewInt32(ctx, *v);
    }
    Any operator()(Float v) {
        return JS_NewFloat64(ctx, *v);
    }
    Any operator()(Bool v) {
        return JS_NewBool(ctx, *v);
    }
    Any operator()(auto) {
        throw std::runtime_error("Invalid conversion to JSValue");
    }
};

using RegVal = std::variant<Int32, Float, Bool, ObjectPtr, StringConst, String, Any>;

template<typename T>
T convert(JSContext* ctx, RegVal v) {
    return std::visit(ConvertVisitor<T>{ctx}, v);
}

inline RegVal convert(JSContext* ctx, RegVal v, ValueType type) {
    switch (type) {
        case ValueType::I32:
            return convert<Int32>(ctx, v);
        case ValueType::Double:
            return convert<Float>(ctx, v);
        case ValueType::Bool:
            return convert<Bool>(ctx, v);
        case ValueType::Object:
            return convert<ObjectPtr>(ctx, v);
        case ValueType::Any:
            return convert<Any>(ctx, v);
        case ValueType::Buffer:
            throw std::runtime_error("Invalid conversion to Buffer");
        case ValueType::String:
            throw std::runtime_error("Not implemented (convert to string)");
        case ValueType::StringConst:
            throw std::runtime_error("Impossible conversion to StringConst");
        case ValueType::Void:
            throw std::runtime_error("Invalid type");
    }
    abort();
}


struct WrapAnyVisitor {
    JSContext* ctx;

    Any operator()(Any v) {
        return v;
    }
    Any operator()(ObjectPtr v) {
        JSValue val = JS_MKPTR(JS_TAG_OBJECT, *v);
        return val;
    }
    Any operator()(Int32 v) {
        return JS_NewInt32(ctx, *v);
    }
    Any operator()(Float v) {
        return JS_NewFloat64(ctx, *v);
    }
    Any operator()(Bool v) {
        return JS_NewBool(ctx, *v);
    }
    Any operator()(auto) {
        throw std::runtime_error("Invalid conversion to JSValue");
    }
};

inline Any wrapAny(JSContext* ctx, RegVal v) {
    return std::visit(WrapAnyVisitor{ctx}, v);
}



namespace detail {

    struct lshift {
        Int32 operator()(int32_t a, uint32_t b) {
            return a << b;
        }
    };

    struct rshift {
        Int32 operator()(int32_t a, uint32_t b) {
            return a >> b;
        }
    };

    struct urshift {
        Int32 operator()(int32_t a, uint32_t b) {
            return static_cast<uint32_t>(a) >> b;
        }
    };

    struct unplus {
        Int32 operator()(I32Able auto a) {
            return +(*a);
        }
        Float operator()(DoubleAble auto a) {
            return +(*a);
        }
        Bool operator()(Bool a) {
            return +(*a);
        }
        Float operator()(auto) {
            throw std::runtime_error("Invalid type");
        }
    };

    struct unminus {
        Int32 operator()(I32Able auto a) {
            return -*a;
        }
        Float operator()(DoubleAble auto a) {
            return -*a;
        }
        Float operator()(auto) {
            throw std::runtime_error("Invalid type");
        }
    };

    struct GetMbrVisitor {
        JSContext* ctx;

        RegVal operator()(JSValue val, StringConst id) {
            JSAtom atom = JS_NewAtom(ctx, (*id).c_str());
            JSValue res = JS_GetProperty(ctx, val, atom);
            JS_FreeAtom(ctx, atom);
            return Any{ res };
        }
        RegVal operator()(JSValue val, Int32 id) {
            JSValue res = JS_GetPropertyUint32(ctx, val, *id);
            return Any{ res };
        }

        RegVal operator()(Any val, auto id) {
            return this->operator()(*val, id);
        }

        RegVal operator()(ObjectPtr val, auto id) {
            return this->operator()(JS_MKPTR(JS_TAG_OBJECT, *val), id);
        }

        RegVal operator()(const auto&, const auto&) {
            throw std::runtime_error("Not implemented (get member access)");
        }
    };

    struct SetMbrVisitor {
        JSContext* ctx;

        int operator()(StringConst id, JSValue val, JSValue parent) {
            JS_DupValue(ctx, val);
            JSAtom atom = JS_NewAtom(ctx, (*id).c_str());
            auto res = JS_SetProperty(ctx, parent, atom, val);
            JS_FreeAtom(ctx, atom);

            return res;
        }
        int operator()(Int32 id, JSValue val, JSValue parent) {
            JS_DupValue(ctx, val);
            auto res = JS_SetPropertyUint32(ctx, parent, *id, val);
            return res;
        }


        int operator()(auto id, Int32 val, JSValue parent) {
            return this->operator()(id, JS_MKVAL(JS_TAG_INT, *val), parent);
        }
        int operator()(auto id, Float val, JSValue parent) {
            JSValue v;
            v.tag = JS_TAG_FLOAT64;
            v.u.float64 = *val;
            return this->operator()(id, v, parent);
        }
        int operator()(auto id, Bool val, JSValue parent) {
            return this->operator()(id, JS_MKVAL(JS_TAG_BOOL, *val), parent);
        }
        int operator()(auto id, ObjectPtr val, JSValue parent) {
            return this->operator()(id, JS_MKPTR(JS_TAG_OBJECT, *val), parent);
        }
        int operator()(auto id, Any val, JSValue parent) {
            return this->operator()(id, *val, parent);
        }


        int operator()(auto id, auto val, ObjectPtr parent) {
            return this->operator()(id, val, JS_MKPTR(JS_TAG_OBJECT, *parent));
        }
        int operator()(auto id, auto val, Any parent) {
            return this->operator()(id, val, *parent);
        }

        int operator()(const auto&, const auto&, const auto&) {
            throw std::runtime_error("Not implemented (set member access)");
        }
    };

}  // namespace detail


inline bool checkType(const RegVal& val, ValueType type) {
    switch (type) {
        case ValueType::I32:
            return std::holds_alternative<Int32>(val);
        case ValueType::Double:
            return std::holds_alternative<Float>(val);
        case ValueType::Bool:
            return std::holds_alternative<Bool>(val);
        case ValueType::Object:
            return std::holds_alternative<ObjectPtr>(val);
        case ValueType::Any:
            return std::holds_alternative<Any>(val);
        case ValueType::Buffer:
            return false;
        case ValueType::String:
            return std::holds_alternative<String>(val);
        case ValueType::StringConst:
            return std::holds_alternative<StringConst>(val);
        case ValueType::Void:
            return false;
    }
    abort();
}


class CFGInterpreter {
    std::map<std::string, CompFn>& _compiledHolder;

    std::unordered_map<int, RegVal> registers;
    std::vector<RegVal> freeStack;

    template<template<typename> class Op, typename Com, typename Res>
    void evalBinop(JSContext*, const Operation& op) {
        setReg(op.res.id, Res{Op<std::decay_t<decltype(*std::declval<Com>())>>{}(
            *std::get<Com>(getReg(op.a.id)),
            *std::get<Com>(getReg(op.b.id))
        )});
    }

    template<template<typename> class Op>
    void evalBinopResType(JSContext* ctx, const Operation& op) {
        switch (op.res.type) {
            case ValueType::I32:
                evalBinop<Op, Int32, Int32>(ctx, op);
                break;
            case ValueType::Double:
                evalBinop<Op, Float, Float>(ctx, op);
                break;
            case ValueType::Bool:
                evalBinop<Op, Bool, Bool>(ctx, op);
                break;
            default:
                throw std::runtime_error("Not implemented (result type)");
        }
    }

    template<template<typename> class Op>
    void evalRelational(JSContext* ctx, const Operation& op) {
        switch (commonUpcast(op.a.type, op.b.type)) {
            case ValueType::I32:
                evalBinop<Op, Int32, Bool>(ctx, op);
                break;
            case ValueType::Double:
                evalBinop<Op, Float, Bool>(ctx, op);
                break;
            case ValueType::Bool:
                evalBinop<Op, Bool, Bool>(ctx, op);
                break;
            default:
                throw std::runtime_error("Not implemented (common upcast)");
        }
    }

    void evalDup(JSContext* ctx, const Operation& op) {
        auto val = getReg(op.a.id);
        std::visit([ctx](auto& v) { v._dup(ctx); }, val);
    }

    void evalPushFree(JSContext*, const Operation& op) {
        freeStack.push_back(getReg(op.a.id));
    }

    void evalRem(JSContext*, const Operation& op) {
        if (op.res.type == ValueType::I32) {
            setReg(op.res.id, Int32{ *std::get<Int32>(getReg(op.a.id)) % *std::get<Int32>(getReg(op.b.id)) });
        }
        else {
            setReg(op.res.id, Float{ std::fmod(*std::get<Float>(getReg(op.a.id)), *std::get<Float>(getReg(op.b.id))) });
        }
    }

    template<typename Op>
    void evalBitShift(JSContext*, const Operation& op) {
        if (op.res.type != ValueType::I32) {
            throw std::runtime_error("Invalid result type");
        }
        setReg(op.res.id, Op{}(  // XXX: polish
            *std::get<Int32>(getReg(op.a.id)),
            static_cast<uint32_t>(*std::get<Int32>(getReg(op.b.id)))
        ));
    }

    void evalSet(JSContext* ctx, const Operation& op) {
        setReg(op.res.id, convert(ctx, getReg(op.a.id), op.res.type));
    }

    template<typename Op>
    void evalUnop(JSContext*, const Operation& op) {
        switch (op.res.type) {
            case ValueType::I32:
                setReg(op.res.id, Op{}(std::get<Int32>(getReg(op.a.id))));
                break;
            case ValueType::Double:
                setReg(op.res.id, Op{}(std::get<Float>(getReg(op.a.id))));
                break;
            case ValueType::Bool:
                setReg(op.res.id, Op{}(std::get<Bool>(getReg(op.a.id))));
                break;
            default:
                throw std::runtime_error("Not implemented (unop)");
        }
    }

    void evalGetMember(JSContext* ctx, const Operation op) {
        auto res = std::visit(detail::GetMbrVisitor{ ctx }, getReg(op.a.id), getReg(op.b.id));
        setReg(op.res.id, res);
    }

    void evalSetMember(JSContext* ctx, const Operation op) {
        auto res = std::visit(detail::SetMbrVisitor{ ctx }, getReg(op.a.id), getReg(op.b.id), getReg(op.res.id));
        if (res < 0) {
            throw std::runtime_error("Failed to set member");
        }
    }

    void evalBoolNot(JSContext*, const Operation& op) {
        setReg(op.res.id, Bool{ !*std::get<Bool>(getReg(op.a.id)) });
    }

    void evalCallNative(JSContext* ctx, const Call& call) {
        auto it = _compiledHolder.find(std::get<Identifier>(call.obj));
        if (it == _compiledHolder.end()) {
            throw std::runtime_error("Function not found");
        }

        std::vector<JSValue> args;
        args.reserve(call.args.size());
        for (auto& arg : call.args) {
            args.push_back(*wrapAny(ctx, getReg(arg.id)));
        }

        auto& fn = it->second;

        cfg::interp::CFGInterpreter interp(_compiledHolder);
        JSValue res = interp.run(fn.fn, ctx, JS_UNDEFINED, args.size(), args.data());

        setReg(call.res.id, convert(ctx, Any{ res }, call.res.type));
    }

    void evalCallFn(JSContext* ctx, const Call& call) {
        auto obj = getReg(std::get<Temp>(call.obj).id);

        std::vector<JSValue> args;
        args.reserve(call.args.size());
        for (auto& arg : call.args) {
            args.push_back(*wrapAny(ctx, getReg(arg.id)));
        }

        JSValue res;
        if (std::holds_alternative<Any>(obj)) {
            res = JS_Call(ctx, *std::get<Any>(obj), JS_UNDEFINED, args.size(), args.data());
        }
        else if (std::holds_alternative<ObjectPtr>(obj)) {
            JSValue objVal = JS_MKPTR(JS_TAG_OBJECT, *std::get<ObjectPtr>(obj));
            res = JS_Call(ctx, objVal, JS_UNDEFINED, args.size(), args.data());
        }
        else {
            throw std::runtime_error("Invalid call operand type");
        }
        setReg(call.res.id, Any{ res });
    }
public:
    CFGInterpreter(std::map<std::string, CompFn>& compiledHolder) : _compiledHolder(compiledHolder) {}

    void evalOperation(JSContext* ctx, const Operation& op) {
        if (op.a.type != ValueType::Void) {
            assert(checkType(getReg(op.a.id), op.a.type) && "Invalid operand type");
        }
        if (op.b.type != ValueType::Void) {
            assert(checkType(getReg(op.b.id), op.b.type) && "Invalid operand type");
        }
        switch (op.op) {
            case Opcode::Add: evalBinopResType<std::plus>(ctx, op); break;
            case Opcode::Sub: evalBinopResType<std::minus>(ctx, op); break;
            case Opcode::Mul: evalBinopResType<std::multiplies>(ctx, op); break;
            case Opcode::Div: evalBinopResType<std::divides>(ctx, op); break;
            case Opcode::Eq: evalRelational<std::equal_to>(ctx, op); break;
            case Opcode::Neq: evalRelational<std::not_equal_to>(ctx, op); break;
            case Opcode::Gt: evalRelational<std::greater>(ctx, op); break;
            case Opcode::Gte: evalRelational<std::greater_equal>(ctx, op); break;
            case Opcode::Lt: evalRelational<std::less>(ctx, op); break;
            case Opcode::Lte: evalRelational<std::less_equal>(ctx, op); break;
            case Opcode::BitAnd: evalBinop<std::bit_and, Int32, Int32>(ctx, op); break;
            case Opcode::BitOr: evalBinop<std::bit_or, Int32, Int32>(ctx, op); break;
            case Opcode::BitXor: evalBinop<std::bit_xor, Int32, Int32>(ctx, op); break;
            case Opcode::Dup: evalDup(ctx, op); break;
            case Opcode::PushFree: evalPushFree(ctx, op); break;
            case Opcode::Rem: evalRem(ctx, op); break;
            case Opcode::LShift: evalBitShift<detail::lshift>(ctx, op); break;
            case Opcode::RShift: evalBitShift<detail::rshift>(ctx, op); break;
            case Opcode::URShift: evalBitShift<detail::urshift>(ctx, op); break;
            case Opcode::Set: evalSet(ctx, op); break;
            case Opcode::UnMinus: evalUnop<detail::unminus>(ctx, op); break;
            case Opcode::UnPlus: evalUnop<detail::unplus>(ctx, op); break;
            case Opcode::GetMember: evalGetMember(ctx, op); break;
            case Opcode::SetMember: evalSetMember(ctx, op); break;
            case Opcode::BoolNot: evalBoolNot(ctx, op); break;
            case Opcode::Pow:
            case Opcode::BitNot:
                throw std::runtime_error("Not implemented (operation) " + std::to_string(static_cast<int>(op.op)));
        }
        if (op.res.type != ValueType::Void) {
            assert(checkType(getReg(op.res.id), op.res.type) && "Invalid result type");
        }
    }

    void constInit(JSContext*, const ConstInit& init) {
        struct visitor {
            RegVal operator()(int32_t value) {
                return Int32{ value };
            }
            RegVal operator()(double value) {
                return Float{ value };
            }
            RegVal operator()(bool value) {
                return Bool{ value };
            }
            RegVal operator()(const std::string& value) {
                return StringConst{ value };
            }
        };

        setReg(init.id, std::visit(visitor{}, init.value));
    }

    void evalStatement(JSContext* ctx, const Statement& s) {
        struct visitor {
            CFGInterpreter& self;
            JSContext* ctx;

            void operator()(const Operation& op) {
                self.evalOperation(ctx, op);
            }
            void operator()(const ConstInit& init) {
                self.constInit(ctx, init);
            }
            void operator()(const Call& call) {
                if (call.isNative()) {
                    self.evalCallNative(ctx, call);
                }
                else {
                    self.evalCallFn(ctx, call);
                }
            }
        };

        std::visit(visitor{ *this, ctx }, s.op);
    }

    RegVal getReg(int id) {
        return registers.at(id);
    }

    void setReg(int id, RegVal value) {
        if (registers.contains(id)) {
            if (registers.at(id).index() != value.index()) {
                throw std::runtime_error("Cannot change type of register");
            }
            registers.at(id) = value;
        }
        else {
            registers.emplace(id, value);
        }
    }

    JSValue run(const Function& func, JSContext* ctx, [[maybe_unused]] JSValueConst this_val, int argc, JSValueConst* argv) {
        registers.clear();
        freeStack.clear();

        std::vector<RegVal> argsHolder;
        argsHolder.reserve(func.args.size());

        Defer d([&] {
            for (auto& val : freeStack) {
                std::visit([ctx](auto& v) { v._free(ctx); }, val);
            }
            for (auto& val : argsHolder) {
                std::visit([ctx](auto& v) { v._free(ctx); }, val);
            }
        });


        for (size_t i = 0; i < func.args.size(); i++) {
            if (i >= static_cast<size_t>(argc)) {
                throw std::runtime_error("Not enough arguments");
            }
            auto set = [&](int id, auto value) {
                setReg(id, value);
                if constexpr (!std::is_same_v<decltype(value), Any> && !std::is_same_v<decltype(value), ObjectPtr>) {
                    argsHolder.push_back(value);
                }
            };
            switch (func.args[i].type) {
                case ValueType::I32: {
                    int32_t value;
                    JS_ToInt32(ctx, &value, argv[i]);
                    set(func.args[i].id, Int32{ value });
                } break;
                case ValueType::Double: {
                    double value;
                    JS_ToFloat64(ctx, &value, argv[i]);
                    set(func.args[i].id, Float{ value });
                } break;
                case ValueType::Bool: {
                    auto val = JS_ToBool(ctx, argv[i]);
                    if (val < 0) {
                        throw std::runtime_error("Invalid conversion to Bool");
                    }
                    set(func.args[i].id, Bool{ val != 0 });
                } break;
                case ValueType::Object: {
                    set(func.args[i].id, ObjectPtr{ JS_VALUE_GET_OBJ(argv[i]) });
                } break;
                case ValueType::Any: {
                    set(func.args[i].id, Any{ argv[i] });
                } break;
                default:
                    throw std::runtime_error("Not implemented (arg type)");
            }
        }

        BasicBlockPtr activeBlock = func.entry;
        while (activeBlock) {
            for (auto& statement : activeBlock->statements) {
                evalStatement(ctx, statement);
            }

            switch (activeBlock->jump.type) {
                case Terminal::Jump:
                    activeBlock = activeBlock->jump.target;
                    break;
                case Terminal::Branch:
                    if (*std::get<Bool>(getReg(activeBlock->jump.value->id))) {
                        activeBlock = activeBlock->jump.target;
                    }
                    else {
                        activeBlock = activeBlock->jump.other;
                    }
                    break;
                case Terminal::Return:
                    return JS_UNDEFINED;
                case Terminal::ReturnValue: {
                    auto val = getReg(activeBlock->jump.value->id);
                    switch (activeBlock->jump.value->type) {
                        case ValueType::I32: {
                            if (auto valPtr = std::get_if<Int32>(&val)) {
                                auto res = *convert<Any>(ctx, val);
                                valPtr->_free(ctx);
                                return res;
                            }
                            throw std::runtime_error("Invalid return type");
                        }
                        case ValueType::Double: {
                            if (auto valPtr = std::get_if<Float>(&val)) {
                                auto res = *convert<Any>(ctx, val);
                                valPtr->_free(ctx);
                                return res;
                            }
                            throw std::runtime_error("Invalid return type");
                        }
                        case ValueType::Bool: {
                            if (auto valPtr = std::get_if<Bool>(&val)) {
                                auto res = *convert<Any>(ctx, val);
                                valPtr->_free(ctx);
                                return res;
                            }
                            throw std::runtime_error("Invalid return type");
                        }
                        case ValueType::Object:
                            if (auto valPtr = std::get_if<ObjectPtr>(&val)) {
                                auto res = *convert<Any>(ctx, val);
                                valPtr->_free(ctx);
                                return res;
                            }
                            throw std::runtime_error("Invalid return type");
                        case ValueType::Any:
                            if (!std::holds_alternative<Any>(val)) {
                                throw std::runtime_error("Invalid return type");
                            }
                            return *std::get<Any>(val);
                        case ValueType::String:
                            throw std::runtime_error("Not implemented (return String)");
                        case ValueType::StringConst:
                            throw std::runtime_error("Not implemented (return StringConst)");
                        case ValueType::Buffer:
                            throw std::runtime_error("Invalid return type");
                        case ValueType::Void:
                            throw std::runtime_error("Invalid return type");
                    }
                    abort();
                }
                case Terminal::Throw:
                    throw std::runtime_error("Not implemented (throw)");
                case Terminal::None:
                    throw std::runtime_error("Invalid jump (None)");
            }
        }

        throw std::runtime_error("No return statement");
    }
};


}  // namespace jac::cfg::interp
