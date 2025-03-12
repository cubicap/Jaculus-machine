#pragma once

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <map>
#include <unordered_map>

#include <jac/machine/compiler/cfg.h>
#include <jac/util.h>
#include <quickjs.h>

#include "types.h"


namespace jac::cfg::interp {


template<typename T>
concept I32Able = std::is_same_v<T, int32_t> || std::is_same_v<T, bool>;

template<typename L>
concept DoubleAble = std::is_same_v<L, double> || I32Able<L>;

struct ObjectPtr {
    void* ptr;
};

// FIXME: use non owning representation for string in testing
// I32, double, bool, object, string const, buffer, any
using RegVal = std::variant<int32_t, double, bool, ObjectPtr, std::string, uint8_t*, JSValue>;

template<typename T>
struct ConvertVisitor {};

template<>
struct ConvertVisitor<int32_t> {
    JSContext* ctx;

    int32_t operator()(DoubleAble auto v) {
        return static_cast<int32_t>(v);
    }
    int32_t operator()(JSValue v) {
        int32_t res;
        if (JS_ToInt32(ctx, &res, v) < 0) {
            throw std::runtime_error("Invalid conversion to Int32");
        }
        return res;
    }
    int32_t operator()(auto) {
        throw std::runtime_error("Invalid conversion to Int32");
    }
};

template<>
struct ConvertVisitor<double> {
    JSContext* ctx;

    double operator()(DoubleAble auto v) {
        return static_cast<double>(v);
    }
    double operator()(auto) {
        throw std::runtime_error("Invalid conversion to Double");
    }
};

template<>
struct ConvertVisitor<bool> {
    JSContext* ctx;

    bool operator()(bool v) {
        return v;
    }
    bool operator()(DoubleAble auto v) {
        return v != 0;
    }
    bool operator()(auto) {
        throw std::runtime_error("Invalid conversion to Bool");
    }
};

template<>
struct ConvertVisitor<ObjectPtr> {
    JSContext* ctx;

    ObjectPtr operator()(ObjectPtr v) {
        return v;
    }
    ObjectPtr operator()(auto) {
        throw std::runtime_error("Invalid conversion to Object");
    }
};

template<>
struct ConvertVisitor<std::string> {
    JSContext* ctx;

    std::string operator()(const std::string& v) {
        return v;
    }
    std::string operator()(auto) {
        throw std::runtime_error("Invalid conversion to StringConst");
    }
};

template<>
struct ConvertVisitor<JSValue> {
    JSContext* ctx;

    JSValue operator()(JSValue v) {
        return v;
    }
    JSValue operator()(ObjectPtr v) {
        return (JSValue) {
            .u = {
                .ptr = v.ptr
            },
            .tag = JS_TAG_OBJECT
        };
    }
    JSValue operator()(int32_t v) {
        return JS_NewInt32(ctx, v);
    }
    JSValue operator()(double v) {
        return JS_NewFloat64(ctx, v);
    }
    JSValue operator()(bool v) {
        return JS_NewBool(ctx, v);
    }
    JSValue operator()(auto) {
        throw std::runtime_error("Invalid conversion to JSValue");
    }
};

template<typename T>
T convert(JSContext* ctx, RegVal v) {
    return std::visit(ConvertVisitor<T>{ctx}, v);
}

inline RegVal convert(JSContext* ctx, RegVal v, ValueType type) {
    switch (type) {
        case ValueType::I32:
            return convert<int32_t>(ctx, v);
        case ValueType::Double:
            return convert<double>(ctx, v);
        case ValueType::Bool:
            return convert<bool>(ctx, v);
        case ValueType::Object:
            return convert<ObjectPtr>(ctx, v);
        case ValueType::Any:
            return convert<JSValue>(ctx, v);
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


namespace detail {

    struct lshift {
        int32_t operator()(int32_t a, uint32_t b) {
            return a << b;
        }
    };

    struct rshift {
        int32_t operator()(int32_t a, uint32_t b) {
            return a >> b;
        }
    };

    struct urshift {
        int32_t operator()(int32_t a, uint32_t b) {
            return static_cast<uint32_t>(a) >> b;
        }
    };

    struct unplus {
        int32_t operator()(I32Able auto a) {
            return +a;
        }
        double operator()(DoubleAble auto a) {
            return +a;
        }
        double operator()(auto) {
            throw std::runtime_error("Invalid type");
        }
    };

    struct unminus {
        int32_t operator()(I32Able auto a) {
            return -a;
        }
        double operator()(DoubleAble auto a) {
            return -a;
        }
        double operator()(auto) {
            throw std::runtime_error("Invalid type");
        }
    };

    struct GetMbrVisitor {
        JSContext* ctx;
        JSValue val;

        RegVal operator()(const std::string& id) {
            JSAtom atom = JS_NewAtom(ctx, id.c_str());
            JSValue res = JS_GetProperty(ctx, val, atom);
            JS_FreeAtom(ctx, atom);
            return res;
        }
        RegVal operator()(const auto&) {
            throw std::runtime_error("Not implemented ([] member access)");
        }
    };

    struct SetMbrVisitor {
        JSContext* ctx;
        JSValue obj;
        JSValue val;

        int operator()(const std::string& id) {
            JSAtom atom = JS_NewAtom(ctx, id.c_str());
            auto res = JS_SetProperty(ctx, obj, atom, val);
            JS_FreeAtom(ctx, atom);

            return res;
        }
        int operator()(const auto&) {
            throw std::runtime_error("Not implemented ([] member access)");
        }
    };

}  // namespace detail


class CFGInterpreter {
    std::map<std::string, CompFn>& _compiledHolder;

    std::unordered_map<int, RegVal> registers;
    std::vector<JSValue> freeStack;

    template<template<typename> class Op, typename Com>
    void evalBinop(JSContext* ctx, const Operation& op) {
        setReg(op.res.id, Op<Com>{}(
            convert<Com>(ctx, getReg(op.a.id)),
            convert<Com>(ctx, getReg(op.b.id))
        ));
    }

    template<template<typename> class Op>
    void evalBinopResType(JSContext* ctx, const Operation& op) {
        switch (op.res.type) {
            case ValueType::I32:
                evalBinop<Op, int32_t>(ctx, op);
                break;
            case ValueType::Double:
                evalBinop<Op, double>(ctx, op);
                break;
            case ValueType::Bool:
                evalBinop<Op, bool>(ctx, op);
                break;
            default:
                throw std::runtime_error("Not implemented (result type)");
        }
    }

    template<template<typename> class Op>
    void evalBinopComType(JSContext* ctx, const Operation& op) {
        switch (commonUpcast(op.a.type, op.b.type)) {
            case ValueType::I32:
                evalBinop<Op, int32_t>(ctx, op);
                break;
            case ValueType::Double:
                evalBinop<Op, double>(ctx, op);
                break;
            case ValueType::Bool:
                evalBinop<Op, bool>(ctx, op);
                break;
            default:
                throw std::runtime_error("Not implemented (common upcast)");
        }
    }

    void evalDup(JSContext* ctx, const Operation& op) {
        if (op.a.type == ValueType::Void || op.a.type == ValueType::I32 || op.a.type == ValueType::Double || op.a.type == ValueType::Bool || op.a.type == ValueType::StringConst) {
            return;  // no op
        }
        if (op.a.type == ValueType::Any || op.a.type == ValueType::Object) {
            auto obj = convert<JSValue>(ctx, getReg(op.a.id));
            JS_DupValue(ctx, obj);
            return;
        }
        throw std::runtime_error("Not implemented (dup)");
    }

    void evalPushFree(JSContext* ctx, const Operation& op) {
        if (op.a.type == ValueType::Void || op.a.type == ValueType::I32 || op.a.type == ValueType::Double || op.a.type == ValueType::Bool || op.a.type == ValueType::StringConst) {
            return;  // no op
        }
        if (op.a.type == ValueType::Any || op.a.type == ValueType::Object) {
            auto obj = convert<JSValue>(ctx, getReg(op.a.id));
            freeStack.push_back(obj);
            return;
        }
        throw std::runtime_error("Not implemented (push free)");
    }

    void evalRem(JSContext* ctx, const Operation& op) {
        if (op.res.type == ValueType::I32) {
            setReg(op.res.id, convert<int32_t>(ctx, getReg(op.a.id)) % convert<int32_t>(ctx, getReg(op.b.id)));
        }
        else {
            setReg(op.res.id, std::fmod(convert<double>(ctx, getReg(op.a.id)), convert<double>(ctx, getReg(op.b.id))));
        }
    }

    template<typename Op>
    void evalBitShift(JSContext* ctx, const Operation& op) {
        if (op.res.type != ValueType::I32) {
            throw std::runtime_error("Invalid result type");
        }
        setReg(op.res.id, Op{}(
            convert<int32_t>(ctx, getReg(op.a.id)),
            static_cast<uint32_t>(convert<int32_t>(ctx, getReg(op.b.id)))
        ));
    }

    void evalSet(JSContext* ctx, const Operation& op) {
        if (op.res.isMember()) {
            auto val = convert<JSValue>(ctx, getReg(op.a.id));
            auto obj = convert<JSValue>(ctx, getReg(op.res.id));
            JS_DupValue(ctx, val);

            auto res = std::visit(
                detail::SetMbrVisitor{ ctx, obj, val },
                getReg(op.res.member->id)
            );
            if (res < 0) {
                JS_FreeValue(ctx, val);
                throw std::runtime_error("Failed to set member");
            }
        }
        else {
            setReg(op.res.id, convert(ctx, getReg(op.a.id), op.res.type));
        }
    }

    template<typename Op>
    void evalUnop(JSContext* ctx, const Operation& op) {
        switch (op.res.type) {
            case ValueType::I32:
                setReg(op.res.id, Op{}(convert<int32_t>(ctx, getReg(op.a.id))));
                break;
            case ValueType::Double:
                setReg(op.res.id, Op{}(convert<double>(ctx, getReg(op.a.id))));
                break;
            case ValueType::Bool:
                setReg(op.res.id, Op{}(convert<bool>(ctx, getReg(op.a.id))));
                break;
            default:
                throw std::runtime_error("Not implemented (unop)");
        }
    }

    void evalGetMember(JSContext* ctx, const Operation op) {
        auto val = convert<JSValue>(ctx, getReg(op.a.id));

        auto res = std::visit(detail::GetMbrVisitor{ ctx, val }, getReg(op.b.id));
        setReg(op.res.id, res);
    }

    void evalBoolNot(JSContext* ctx, const Operation& op) {
        setReg(op.res.id, !convert<bool>(ctx, getReg(op.a.id)));
    }

    void evalCallNative(JSContext* ctx, const Call& call) {
        auto it = _compiledHolder.find(std::get<Identifier>(call.obj));
        if (it == _compiledHolder.end()) {
            throw std::runtime_error("Function not found");
        }

        std::vector<JSValue> args;
        args.reserve(call.args.size());
        for (auto& arg : call.args) {
            args.push_back(convert<JSValue>(ctx, getReg(arg.id)));
        }

        auto& fn = it->second;

        cfg::interp::CFGInterpreter interp(_compiledHolder);
        JSValue res = interp.run(fn.fn, ctx, JS_UNDEFINED, args.size(), args.data());

        setReg(call.res.id, res);
    }
public:
    CFGInterpreter(std::map<std::string, CompFn>& compiledHolder) : _compiledHolder(compiledHolder) {}

    void evalOperation(JSContext* ctx, const Operation& op) {
        switch (op.op) {
            case Opcode::Add: evalBinopResType<std::plus>(ctx, op); break;
            case Opcode::Sub: evalBinopResType<std::minus>(ctx, op); break;
            case Opcode::Mul: evalBinopResType<std::multiplies>(ctx, op); break;
            case Opcode::Div: evalBinopResType<std::divides>(ctx, op); break;
            case Opcode::Eq: evalBinopComType<std::equal_to>(ctx, op); break;
            case Opcode::Neq: evalBinopComType<std::not_equal_to>(ctx, op); break;
            case Opcode::Gt: evalBinopComType<std::greater>(ctx, op); break;
            case Opcode::Gte: evalBinopComType<std::greater_equal>(ctx, op); break;
            case Opcode::Lt: evalBinopComType<std::less>(ctx, op); break;
            case Opcode::Lte: evalBinopComType<std::less_equal>(ctx, op); break;
            case Opcode::BitAnd: evalBinop<std::bit_and, int32_t>(ctx, op); break;
            case Opcode::BitOr: evalBinop<std::bit_or, int32_t>(ctx, op); break;
            case Opcode::BitXor: evalBinop<std::bit_xor, int32_t>(ctx, op); break;
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
            case Opcode::BoolNot: evalBoolNot(ctx, op); break;
            case Opcode::Pow:
            case Opcode::BitNot:
                throw std::runtime_error("Not implemented (operation) " + std::to_string(static_cast<int>(op.op)));
        }
    }

    void constInit(JSContext*, const ConstInit& init) {
        struct visitor {
            RegVal operator()(int value) {
                return value;
            }
            RegVal operator()(double value) {
                return value;
            }
            RegVal operator()(bool value) {
                return value;
            }
            RegVal operator()(const std::string& value) {
                return value;
            }
        };

        setReg(init.id, std::visit(visitor{}, init.value));
    }

    void evalStatement(JSContext* ctx, Statement s) {
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
                    throw std::runtime_error("Not implemented (call)");
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
            if (registers[id].index() != value.index()) {
                throw std::runtime_error("Cannot change type of register");
            }
            registers[id] = value;
        }
        else {
            registers.emplace(id, value);
        }
    }

    JSValue run(const Function& func, JSContext* ctx, [[maybe_unused]] JSValueConst this_val, int argc, JSValueConst* argv) {
        registers.clear();
        freeStack.clear();

        Defer d([&] {
            for (auto& val : freeStack) {
                JS_FreeValue(ctx, val);
            }
        });

        for (size_t i = 0; i < func.args.size(); i++) {
            if (i >= static_cast<size_t>(argc)) {
                throw std::runtime_error("Not enough arguments");
            }
            switch (func.args[i].type) {
                case ValueType::I32: {
                    int32_t value;
                    JS_ToInt32(ctx, &value, argv[i]);
                    setReg(func.args[i].id, value);
                } break;
                case ValueType::Double: {
                    double value;
                    JS_ToFloat64(ctx, &value, argv[i]);
                    setReg(func.args[i].id, value);
                } break;
                case ValueType::Bool: {
                    setReg(func.args[i].id, JS_ToBool(ctx, argv[i]));
                } break;
                case ValueType::Object: {
                    setReg(func.args[i].id, ObjectPtr{ JS_VALUE_GET_PTR(argv[i]) });
                } break;
                case ValueType::Any: {
                    setReg(func.args[i].id, argv[i]);
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
                    if (std::get<bool>(getReg(activeBlock->jump.value->id))) {
                        activeBlock = activeBlock->jump.target;
                    }
                    else {
                        activeBlock = activeBlock->jump.other;
                    }
                    break;
                case Terminal::Return:
                    return JS_UNDEFINED;
                case Terminal::ReturnValue: {
                    switch (activeBlock->jump.value->type) {
                        case ValueType::I32:
                            return convert<JSValue>(ctx, convert<int32_t>(ctx, getReg(activeBlock->jump.value->id)));
                            break;
                        case ValueType::Double:
                            return convert<JSValue>(ctx, convert<double>(ctx, getReg(activeBlock->jump.value->id)));
                            break;
                        case ValueType::Bool:
                            return convert<JSValue>(ctx, convert<bool>(ctx, getReg(activeBlock->jump.value->id)));
                            break;
                        case ValueType::Object:
                            return convert<JSValue>(ctx, convert<ObjectPtr>(ctx, getReg(activeBlock->jump.value->id)));
                            break;
                        case ValueType::Any:
                            return convert<JSValue>(ctx, getReg(activeBlock->jump.value->id));
                            break;
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
