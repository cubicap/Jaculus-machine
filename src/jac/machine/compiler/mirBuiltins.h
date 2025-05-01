#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <ranges>
#include <string>
#include <vector>

#include <mir.h>
#include <quickjs.h>

#include "opcode.h"
#include "quickjsOps.h"


namespace jac::cfg::mir_emit {


enum class ErrorType : int32_t {
    SyntaxError,
    TypeError,
    ReferenceError,
    RangeError,
    InternalError
};


// an implementation of stack as a linked list with expanding blocks
// to reduce allocations and reallocations
template<typename T>
class BlockStack {
    struct Node {
        std::vector<T> data;
        std::unique_ptr<Node> next;

        Node(size_t size) {
            data.reserve(size);
        }
    };

    std::unique_ptr<Node> head;
    Node* tail;

public:
    struct Iterator {
        Node* node;
        size_t index;

        Iterator(Node* node_, size_t index_): node(node_), index(index_) {}

        T& operator*() {
            return node->data[index];
        }
        Iterator& operator++() {
            if (++index == node->data.size()) {
                node = node->next.get();
                index = 0;
            }
            return *this;
        }
        bool operator!=(const Iterator& other) const {
            return node != other.node || index != other.index;
        }
    };

    BlockStack(): head(nullptr), tail(nullptr) {}

    void push_back(T value) {
        if (!head) {
            head = std::make_unique<Node>(4);
            tail = head.get();
        }
        else if (tail->data.size() == tail->data.capacity()) {
            tail->next = std::make_unique<Node>(tail->data.capacity() * 2);
            tail = tail->next.get();
        }
        tail->data.emplace_back(value);
    }

    Iterator begin() {
        return Iterator(head.get(), 0);
    }
    Iterator end() {
        return Iterator(nullptr, 0);
    }
};

struct RuntimeContext {
    JSContext* ctx;
    int32_t exceptionFlag = 0;

    std::vector<BlockStack<JSValue>> freeStackFrames;
    std::vector<std::unique_ptr<char[]>> stringConsts;
};


inline void pushFreeStackFrame(RuntimeContext* rtCtx) {
    rtCtx->freeStackFrames.emplace_back();
}
inline void popFreeStackFrame(RuntimeContext* rtCtx) {
    for (auto& val : rtCtx->freeStackFrames.back()) {
        JS_FreeValue(rtCtx->ctx, val);
    }
    rtCtx->freeStackFrames.pop_back();
}
inline void pushFreeImpl(RuntimeContext* rtCtx, JSValue value) {
    if (!JS_VALUE_HAS_REF_COUNT(value)) {
        return;
    }
    rtCtx->freeStackFrames.back().push_back(value);
}


struct NativeFunction {
    MIR_item_t func;
    MIR_item_t prototype;
    std::vector<ValueType> args;
    ValueType ret;
    bool mayThrow;
};


struct Builtins {
    RuntimeContext* rtCtx;

    std::map<std::string, MIR_item_t> prototypes;
    std::map<std::string, NativeFunction> functions;

    NativeFunction fn(std::string name) {
        auto it = functions.find(name);
        if (it == functions.end()) {
            throw std::runtime_error("Function not found " + name);
        }
        return it->second;
    }
};


inline std::pair<MIR_type_t, size_t> getMIRArgType(ValueType type) {
    switch (type) {
        case ValueType::I32:
            return { MIR_T_I32, 0 };
        case ValueType::F64:
            return { MIR_T_D, 0 };
        case ValueType::Bool:
            return { MIR_T_I32, 0 };
        case ValueType::Object:
            return { MIR_T_P, 0 };
        case ValueType::StringConst:
            return { MIR_T_P, 0 };
        case ValueType::Any:
            return { static_cast<MIR_type_t>(MIR_T_BLK + 1), sizeof(JSValue) };
        default:
            throw std::runtime_error("Invalid MIR type");
    }
}

inline MIR_type_t getMIRRetType(ValueType type) {
    auto [mirType, size] = getMIRArgType(type);
    if (MIR_all_blk_type_p(mirType)) {
        throw std::runtime_error("Invalid return type");
    }

    return mirType;
}

inline MIR_type_t getMIRRegType(ValueType type) {
    if (isIntegral(type) || type == ValueType::Object || type == ValueType::StringConst) {
        return MIR_T_I64;
    }
    if (type == ValueType::F64) {
        return MIR_T_D;
    }
    throw std::runtime_error("Invalid reg type");
}


inline bool hasRetArg(ValueType ret) {
    return ret == ValueType::Any;
}


// output arguments to prevent relocation of the strings
inline void getArgs(const auto& args, ValueType res, std::vector<std::string>& argNamesOut, std::vector<MIR_var_t>& argsOut) {
    bool retArg = hasRetArg(res);

    argNamesOut.reserve(args.size() + retArg);
    argsOut.reserve(argsOut.size() + args.size() + retArg);
    for (const auto& [arg, name] : args) {
        argNamesOut.emplace_back(name);
        auto [type, size] = getMIRArgType(arg);
        MIR_var_t var = {
            .type = type,
            .name = argNamesOut.back().c_str(),
            .size = size
        };
        argsOut.emplace_back(var);
    }

    if (retArg) {
        MIR_var_t var = {
            .type = MIR_T_P,
            .name = "res"
        };
        argsOut.emplace_back(var);
    }
}


inline std::string getMIRTypeId(MIR_type_t type, size_t size) {
    if (type == MIR_T_I32 || (type == MIR_T_P && sizeof(void*) == sizeof(int32_t))) {
        return "I32";
    }
    else if (type == MIR_T_I64 || (type == MIR_T_P && sizeof(void*) == sizeof(int64_t))) {
        return "I64";
    }
    else if (type == MIR_T_D) {
        return "D";
    }
    else if (MIR_blk_type_p(type)) {
        return "BLK" + std::to_string(size);
    }
    else if (type == MIR_T_P) {
        return "P";
    }
    throw std::runtime_error("Invalid MIR type");
}


inline std::string encodeSignature(const std::vector<ValueType>& args, ValueType res) {
    std::string signature;
    if (hasRetArg(res) || res == ValueType::Void) {
        signature += "V";
    }
    else {
        auto [type, size] = getMIRArgType(res);
        signature += getMIRTypeId(type, size);
    }
    signature += "_";

    for (const auto& arg : args) {
        auto [type, size] = getMIRArgType(arg);
        signature += getMIRTypeId(type, size);
    }
    if (hasRetArg(res)) {
        signature += getMIRTypeId(MIR_T_P, 0);
    }
    return signature;
}


inline MIR_item_t getProto(MIR_context_t ctx, Builtins& builtins, std::vector<ValueType> args, ValueType res) {
    auto sgn = encodeSignature(args, res);
    {
        auto it = builtins.prototypes.find(sgn);
        if (it != builtins.prototypes.end()) {
            return it->second;
        }
    }

    std::vector<std::string> argNames;
    std::vector<MIR_var_t> mirArgs{ { .type = MIR_T_P, .name = "rtCtx" }};
    getArgs(std::views::transform(
        std::views::iota(static_cast<size_t>(0), args.size()),
        [&](auto i) { return std::pair{ args[i], "arg" + std::to_string(i) }; }
    ), res, argNames, mirArgs);

    std::string name = "_proto_" + sgn;

    MIR_item_t proto;

    if (hasRetArg(res) || res == ValueType::Void) {
        proto = MIR_new_proto_arr(ctx, name.c_str(), 0, nullptr, mirArgs.size(), mirArgs.data());
    }
    else {
        auto retType = getMIRRetType(res);
        proto = MIR_new_proto_arr(ctx, name.c_str(), 1, &retType, mirArgs.size(), mirArgs.data());
    }

    builtins.prototypes.emplace(sgn, proto);
    return proto;
}


inline void addNativeFunction(MIR_context_t ctx, Builtins& builtins, std::string name, std::vector<ValueType> args, ValueType ret, bool mayThrow, auto func) {
    auto proto = getProto(ctx, builtins, args, ret);
    MIR_load_external(ctx, name.c_str(), reinterpret_cast<void*>(func));  // NOLINT
    auto funcItem = MIR_new_import(ctx, name.c_str());

    builtins.functions.emplace(name, NativeFunction{
        .func = funcItem,
        .prototype = proto,
        .args = args,
        .ret = ret,
        .mayThrow = mayThrow
    });
}


inline Builtins generateBuiltins(MIR_context_t ctx, RuntimeContext* rtCtx) {
    using JSObject = void;

    Builtins builtins{
        .rtCtx = rtCtx,
        .functions = {}
    };

    addNativeFunction(ctx, builtins, "__dupVal", { ValueType::Any }, ValueType::Void, false,
        +[](RuntimeContext* ctx_, JSValue a) {
            JS_DupValue(ctx_->ctx, a);
        }
    );
    addNativeFunction(ctx, builtins, "__pushFreeVal", { ValueType::Any }, ValueType::Void, false, pushFreeImpl);
    addNativeFunction(ctx, builtins, "__dupObj", { ValueType::Object }, ValueType::Void, false,
        +[](RuntimeContext* ctx_, JSObject* obj) {
            JSValue val = JS_MKPTR(JS_TAG_OBJECT, obj);
            JS_DupValue(ctx_->ctx, val);
        }
    );
    addNativeFunction(ctx, builtins, "__pushFreeObj", { ValueType::Object }, ValueType::Void, false,
        +[](RuntimeContext* ctx_, JSObject* obj) {
            JSValue val = JS_MKPTR(JS_TAG_OBJECT, obj);
            pushFreeImpl(ctx_, val);
        }
    );

    addNativeFunction(ctx, builtins, "__enterStackFrame", {}, ValueType::Void, false,
        +[](RuntimeContext* ctx_) {
            pushFreeStackFrame(ctx_);
        }
    );
    addNativeFunction(ctx, builtins, "__exitStackFrame", {}, ValueType::Void, false,
        +[](RuntimeContext* ctx_) {
            popFreeStackFrame(ctx_);
        }
    );

    addNativeFunction(ctx, builtins, "__getMemberObjCStr", { ValueType::Object, ValueType::StringConst }, ValueType::Any, true,
        +[](RuntimeContext* ctx_, JSObject* obj, const char* name, JSValue* res) {
            JSValue val = JS_MKPTR(JS_TAG_OBJECT, obj);
            *res = JS_GetPropertyStr(ctx_->ctx, val, name);
            if (JS_IsException(*res)) {
                ctx_->exceptionFlag = 1;
            }
        }
    );
    addNativeFunction(ctx, builtins, "__setMemberObjCStr", { ValueType::Object, ValueType::StringConst, ValueType::Any }, ValueType::Void, true,
        +[](RuntimeContext* ctx_, JSObject* obj, const char* name, JSValue val) {
            JSValue objVal = JS_MKPTR(JS_TAG_OBJECT, obj);
            if (JS_SetPropertyStr(ctx_->ctx, objVal, name, val) < 0) {
                ctx_->exceptionFlag = 1;
            }
        }
    );
    addNativeFunction(ctx, builtins, "__getMemberObjI32", { ValueType::Object, ValueType::I32 }, ValueType::Any, true,
        +[](RuntimeContext* ctx_, JSObject* obj, int32_t index, JSValue* res) {
            JSValue val = JS_MKPTR(JS_TAG_OBJECT, obj);
            *res = JS_GetPropertyUint32(ctx_->ctx, val, index);
            if (JS_IsException(*res)) {
                ctx_->exceptionFlag = 1;
            }
        }
    );
    addNativeFunction(ctx, builtins, "__setMemberObjI32", { ValueType::Object, ValueType::I32, ValueType::Any }, ValueType::Void, true,
        +[](RuntimeContext* ctx_, JSObject* obj, int32_t index, JSValue val) {
            JSValue objVal = JS_MKPTR(JS_TAG_OBJECT, obj);
            if (JS_SetPropertyUint32(ctx_->ctx, objVal, index, val) < 0) {
                ctx_->exceptionFlag = 1;
            }
        }
    );

    static constexpr auto callAnyAny = +[](RuntimeContext* ctx_, JSValue obj, JSValue this_, int32_t argc, JSValue* argv) {
        JSValue res = JS_Call(ctx_->ctx, obj, this_, argc, argv);
        if (JS_IsException(res)) {
            ctx_->exceptionFlag = 1;
            return;
        }
        argv[0] = res;
    };

    addNativeFunction(ctx, builtins, "__callAnyAny", { ValueType::Any, ValueType::Any, ValueType::I32 }, ValueType::Any, true, callAnyAny);
    addNativeFunction(ctx, builtins, "__callObjAny", { ValueType::Object, ValueType::Any, ValueType::I32 }, ValueType::Any, true,
        +[](RuntimeContext* ctx_, JSObject* obj, JSValue this_, int32_t argc, JSValue* argv) {
            callAnyAny(ctx_, JS_MKPTR(JS_TAG_OBJECT, obj), this_, argc, argv);
        }
    );
    addNativeFunction(ctx, builtins, "__callAnyObj", { ValueType::Any, ValueType::Object, ValueType::I32 }, ValueType::Any, true,
        +[](RuntimeContext* ctx_, JSValue obj, JSObject* this_, int32_t argc, JSValue* argv) {
            callAnyAny(ctx_, obj, JS_MKPTR(JS_TAG_OBJECT, this_), argc, argv);
        }
    );
    addNativeFunction(ctx, builtins, "__callObjObj", { ValueType::Object, ValueType::Object, ValueType::I32 }, ValueType::Any, true,
        +[](RuntimeContext* ctx_, JSObject* obj, JSObject* this_, int32_t argc, JSValue* argv) {
            callAnyAny(ctx_, JS_MKPTR(JS_TAG_OBJECT, obj), JS_MKPTR(JS_TAG_OBJECT, this_), argc, argv);
        }
    );
    addNativeFunction(ctx, builtins, "__callAnyUndefined", { ValueType::Any, ValueType::I32 }, ValueType::Any, true,
        +[](RuntimeContext* ctx_, JSValue obj, int32_t argc, JSValue* argv) {
            callAnyAny(ctx_, obj, JS_UNDEFINED, argc, argv);
        }
    );
    addNativeFunction(ctx, builtins, "__callObjUndefined", { ValueType::Object, ValueType::I32 }, ValueType::Any, true,
        +[](RuntimeContext* ctx_, JSObject* obj, int32_t argc, JSValue* argv) {
            callAnyAny(ctx_, JS_MKPTR(JS_TAG_OBJECT, obj), JS_UNDEFINED, argc, argv);
        }
    );

    addNativeFunction(ctx, builtins, "__add", { ValueType::Any, ValueType::Any }, ValueType::Any, true,
        +[](RuntimeContext* ctx_, JSValue a, JSValue b, JSValue* res) {
            *res = quickjs_ops::add(ctx_->ctx, a, b, &ctx_->exceptionFlag);
        }
    );
    addNativeFunction(ctx, builtins, "__sub", { ValueType::Any, ValueType::Any }, ValueType::Any, true,
        +[](RuntimeContext* ctx_, JSValue a, JSValue b, JSValue* res) {
            *res = quickjs_ops::sub(ctx_->ctx, a, b, &ctx_->exceptionFlag);
        }
    );
    addNativeFunction(ctx, builtins, "__mul", { ValueType::Any, ValueType::Any }, ValueType::Any, true,
        +[](RuntimeContext* ctx_, JSValue a, JSValue b, JSValue* res) {
            *res = quickjs_ops::mul(ctx_->ctx, a, b, &ctx_->exceptionFlag);
        }
    );
    addNativeFunction(ctx, builtins, "__div", { ValueType::Any, ValueType::Any }, ValueType::Any, true,
        +[](RuntimeContext* ctx_, JSValue a, JSValue b, JSValue* res) {
            *res = quickjs_ops::div(ctx_->ctx, a, b, &ctx_->exceptionFlag);
        }
    );
    addNativeFunction(ctx, builtins, "__rem", { ValueType::Any, ValueType::Any }, ValueType::Any, true,
        +[](RuntimeContext* ctx_, JSValue a, JSValue b, JSValue* res) {
            *res = quickjs_ops::rem(ctx_->ctx, a, b, &ctx_->exceptionFlag);
        }
    );

    addNativeFunction(ctx, builtins, "__powF64", { ValueType::F64, ValueType::F64 }, ValueType::F64, false,
        +[](RuntimeContext* ctx_, double a, double b) -> double {
            return std::pow(a, b);
        }
    );

    addNativeFunction(ctx, builtins, "__convertI32", { ValueType::Any }, ValueType::I32, true,
        +[](RuntimeContext* ctx_, JSValue a) -> int32_t {
            int32_t res;
            if (JS_ToInt32(ctx_->ctx, &res, a)) {
                ctx_->exceptionFlag = 1;
                return 0;
            }
            return res;
        }
    );
    addNativeFunction(ctx, builtins, "__convertF64", { ValueType::Any }, ValueType::F64, true,
        +[](RuntimeContext* ctx_, JSValue a) -> double {
            double res;
            if (JS_ToFloat64(ctx_->ctx, &res, a)) {
                ctx_->exceptionFlag = 1;
                return 0;
            }
            return res;
        }
    );

    addNativeFunction(ctx, builtins, "__throwError", { ValueType::StringConst, ValueType::I32 }, ValueType::Void, true,
        +[](RuntimeContext* ctx_, const char* msg, int32_t type) {
            switch (static_cast<ErrorType>(type)) {
                case ErrorType::SyntaxError:     JS_ThrowSyntaxError(ctx_->ctx, "%s", msg);     break;
                case ErrorType::TypeError:       JS_ThrowTypeError(ctx_->ctx, "%s", msg);       break;
                case ErrorType::ReferenceError:  JS_ThrowReferenceError(ctx_->ctx, "%s", msg);  break;
                case ErrorType::RangeError:      JS_ThrowRangeError(ctx_->ctx, "%s", msg);      break;
                case ErrorType::InternalError:   JS_ThrowInternalError(ctx_->ctx, "%s", msg);   break;
                default: throw std::runtime_error("Invalid error type");
            }
            ctx_->exceptionFlag = 1;
        }
    );
    addNativeFunction(ctx, builtins, "__throwVal", { ValueType::Any }, ValueType::Void, true,
        +[](RuntimeContext* ctx_, JSValue a) {
            JS_Throw(ctx_->ctx, a);
            ctx_->exceptionFlag = 1;
        }
    );

    return builtins;
}


}  // namespace jac::cfg::mir_emit
