#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <mir.h>
#include <quickjs.h>

#include "opcode.h"
#include "quickjsOps.h"


namespace jac::cfg::mir_emit {


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
};


struct Builtins {
    RuntimeContext* rtCtx;

    std::map<std::string, MIR_item_t> prototypes;
    std::map<std::string, NativeFunction> functions;

    NativeFunction fn(std::string name) {
        auto it = functions.find(name);
        if (it == functions.end()) {
            throw std::runtime_error("Function not found");
        }
        return it->second;
    }
};


inline std::pair<MIR_type_t, size_t> getMIRArgType(ValueType type) {
    switch (type) {
        case ValueType::I32:
            return { MIR_T_I32, 0 };
        case ValueType::Double:
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
    if (type == ValueType::Double) {
        return MIR_T_D;
    }
    throw std::runtime_error("Invalid reg type");
}


inline bool hasRetArg(ValueType ret) {
    return ret == ValueType::Any;
}


// output arguments to prevent relocation of the strings
inline void getArgs(std::vector<ValueType> args, ValueType res, std::vector<std::string>& argNamesOut, std::vector<MIR_var_t>& argsOut) {
    bool retArg = hasRetArg(res);

    argNamesOut.reserve(args.size() + retArg);
    argsOut.reserve(argsOut.size() + args.size() + retArg);
    int i = 0;
    for (const auto& arg : args) {
        argNamesOut.emplace_back("_" + std::to_string(i));
        i++;
        auto [type, size] = getMIRArgType(arg);
        MIR_var_t var = {
            .type = type,
            .name = argNamesOut.back().c_str(),
            .size = size
        };
        argsOut.emplace_back(var);
    }

    if (retArg) {
        argNamesOut.emplace_back("res");
        MIR_var_t var = {
            .type = MIR_T_P,
            .name = argNamesOut.back().c_str()
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
    getArgs(args, res, argNames, mirArgs);

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


// if res is not void, the last arg is the return target
inline void insertCall(MIR_context_t ctx, MIR_item_t fun, Builtins& builtins, std::string ident, std::vector<MIR_reg_t> ops) {
    auto func = builtins.fn(ident);
    auto args = func.args;
    auto res = func.ret;
    auto proto = func.prototype;
    auto callable = func.func;
    auto rtCtx = builtins.rtCtx;

    bool retArg = hasRetArg(res);
    if ((args.size() + (res != ValueType::Void)) != ops.size()) {
        throw std::runtime_error("Invalid number of arguments");
    }

    std::vector<MIR_op_t> callArgs;
    callArgs.push_back(MIR_new_ref_op(ctx, proto));
    callArgs.push_back(MIR_new_ref_op(ctx, callable));
    if (res != ValueType::Void && !retArg) {
        callArgs.push_back(MIR_new_reg_op(ctx, ops.back()));
    }

    callArgs.push_back(MIR_new_int_op(ctx, reinterpret_cast<int64_t>(rtCtx)));  // NOLINT
    for (size_t i = 0; i < args.size(); ++i) {
        auto [type, size] = getMIRArgType(args[i]);
        if (MIR_blk_type_p(type)) {
            callArgs.push_back(MIR_new_mem_op(ctx, type, size, ops[i], 0, 0));
        }
        else {
            callArgs.push_back(MIR_new_reg_op(ctx, ops[i]));
        }
    }
    if (retArg) {
        callArgs.push_back(MIR_new_reg_op(ctx, ops.back()));
    }
    MIR_append_insn(ctx, fun, MIR_new_insn_arr(ctx, MIR_CALL, callArgs.size(), callArgs.data()));
}


inline void addNativeFunction(MIR_context_t ctx, Builtins& builtins, std::string name, std::vector<ValueType> args, ValueType ret, auto func) {
    auto proto = getProto(ctx, builtins, args, ret);
    MIR_load_external(ctx, name.c_str(), reinterpret_cast<void*>(func));  // NOLINT
    auto funcItem = MIR_new_import(ctx, name.c_str());

    builtins.functions.emplace(name, NativeFunction{
        .func = funcItem,
        .prototype = proto,
        .args = args,
        .ret = ret
    });
}


inline Builtins generateBuiltins(MIR_context_t ctx, RuntimeContext* rtCtx) {
    Builtins builtins{
        .rtCtx = rtCtx,
        .functions = {}
    };

    addNativeFunction(ctx, builtins, "__dupVal", { ValueType::Any }, ValueType::Void,
        +[](RuntimeContext* ctx_, JSValue a) {
            JS_DupValue(ctx_->ctx, a);
        }
    );
    addNativeFunction(ctx, builtins, "__pushFreeVal", { ValueType::Any }, ValueType::Void, pushFreeImpl);
    addNativeFunction(ctx, builtins, "__dupObj", { ValueType::Object }, ValueType::Void,
        +[](RuntimeContext* ctx_, JSObject* obj) {
            JSValue val = JS_MKPTR(JS_TAG_OBJECT, obj);
            JS_DupValue(ctx_->ctx, val);
        }
    );
    addNativeFunction(ctx, builtins, "__pushFreeObj", { ValueType::Object }, ValueType::Void,
        +[](RuntimeContext* ctx_, JSObject* obj) {
            JSValue val = JS_MKPTR(JS_TAG_OBJECT, obj);
            pushFreeImpl(ctx_, val);
        }
    );

    addNativeFunction(ctx, builtins, "__enterStackFrame", {}, ValueType::Void,
        +[](RuntimeContext* ctx_) {
            pushFreeStackFrame(ctx_);
        }
    );
    addNativeFunction(ctx, builtins, "__exitStackFrame", {}, ValueType::Void,
        +[](RuntimeContext* ctx_) {
            popFreeStackFrame(ctx_);
        }
    );

    addNativeFunction(ctx, builtins, "__getMemberObjCStr", { ValueType::Object, ValueType::StringConst }, ValueType::Any,
        +[](RuntimeContext* ctx_, JSObject* obj, const char* name, JSValue* res) {
            JSValue val = JS_MKPTR(JS_TAG_OBJECT, obj);
            *res = JS_GetPropertyStr(ctx_->ctx, val, name);
        }
    );
    addNativeFunction(ctx, builtins, "__setMemberObjCStr", { ValueType::Object, ValueType::StringConst, ValueType::Any }, ValueType::Void,
        +[](RuntimeContext* ctx_, JSObject* obj, const char* name, JSValue val) {
            JSValue objVal = JS_MKPTR(JS_TAG_OBJECT, obj);
            JS_SetPropertyStr(ctx_->ctx, objVal, name, val);
        }
    );
    addNativeFunction(ctx, builtins, "__getMemberObjI32", { ValueType::Object, ValueType::I32 }, ValueType::Any,
        +[](RuntimeContext* ctx_, JSObject* obj, int32_t index, JSValue* res) {
            JSValue val = JS_MKPTR(JS_TAG_OBJECT, obj);
            *res = JS_GetPropertyUint32(ctx_->ctx, val, index);
        }
    );
    addNativeFunction(ctx, builtins, "__setMemberObjI32", { ValueType::Object, ValueType::I32, ValueType::Any }, ValueType::Void,
        +[](RuntimeContext* ctx_, JSObject* obj, int32_t index, JSValue val) {
            JSValue objVal = JS_MKPTR(JS_TAG_OBJECT, obj);
            JS_SetPropertyUint32(ctx_->ctx, objVal, index, val);
        }
    );

    static constexpr auto callAnyAny = +[](RuntimeContext* ctx_, JSValue obj, JSValue this_, int32_t argc, JSValue* argv) {
        JSValue res = JS_Call(ctx_->ctx, obj, this_, argc, argv);
        if (JS_IsException(res)) {
            // TODO: handle exception
        }
        argv[0] = res;
    };

    addNativeFunction(ctx, builtins, "__callAnyAny", { ValueType::Any, ValueType::Any, ValueType::I32 }, ValueType::Any, callAnyAny);
    addNativeFunction(ctx, builtins, "__callObjAny", { ValueType::Object, ValueType::Any, ValueType::I32 }, ValueType::Any,
        +[](RuntimeContext* ctx_, JSObject* obj, JSValue this_, int32_t argc, JSValue* argv) {
            callAnyAny(ctx_, JS_MKPTR(JS_TAG_OBJECT, obj), this_, argc, argv);
        }
    );
    addNativeFunction(ctx, builtins, "__callAnyObj", { ValueType::Any, ValueType::Object, ValueType::I32 }, ValueType::Any,
        +[](RuntimeContext* ctx_, JSValue obj, JSObject* this_, int32_t argc, JSValue* argv) {
            callAnyAny(ctx_, obj, JS_MKPTR(JS_TAG_OBJECT, this_), argc, argv);
        }
    );
    addNativeFunction(ctx, builtins, "__callObjObj", { ValueType::Object, ValueType::Object, ValueType::I32 }, ValueType::Any,
        +[](RuntimeContext* ctx_, JSObject* obj, JSObject* this_, int32_t argc, JSValue* argv) {
            callAnyAny(ctx_, JS_MKPTR(JS_TAG_OBJECT, obj), JS_MKPTR(JS_TAG_OBJECT, this_), argc, argv);
        }
    );
    addNativeFunction(ctx, builtins, "__callAnyUndefined", { ValueType::Any, ValueType::I32 }, ValueType::Any,
        +[](RuntimeContext* ctx_, JSValue obj, int32_t argc, JSValue* argv) {
            callAnyAny(ctx_, obj, JS_UNDEFINED, argc, argv);
        }
    );
    addNativeFunction(ctx, builtins, "__callObjUndefined", { ValueType::Object, ValueType::I32 }, ValueType::Any,
        +[](RuntimeContext* ctx_, JSObject* obj, int32_t argc, JSValue* argv) {
            callAnyAny(ctx_, JS_MKPTR(JS_TAG_OBJECT, obj), JS_UNDEFINED, argc, argv);
        }
    );

    addNativeFunction(ctx, builtins, "__add", { ValueType::Any, ValueType::Any }, ValueType::Any,
        +[](RuntimeContext* ctx_, JSValue a, JSValue b, JSValue* res) {
            *res = quickjs_ops::add(ctx_->ctx, a, b);
        }
    );
    addNativeFunction(ctx, builtins, "__sub", { ValueType::Any, ValueType::Any }, ValueType::Any,
        +[](RuntimeContext* ctx_, JSValue a, JSValue b, JSValue* res) {
            *res = quickjs_ops::sub(ctx_->ctx, a, b);
        }
    );
    addNativeFunction(ctx, builtins, "__mul", { ValueType::Any, ValueType::Any }, ValueType::Any,
        +[](RuntimeContext* ctx_, JSValue a, JSValue b, JSValue* res) {
            *res = quickjs_ops::mul(ctx_->ctx, a, b);
        }
    );
    addNativeFunction(ctx, builtins, "__div", { ValueType::Any, ValueType::Any }, ValueType::Any,
        +[](RuntimeContext* ctx_, JSValue a, JSValue b, JSValue* res) {
            *res = quickjs_ops::div(ctx_->ctx, a, b);
        }
    );
    addNativeFunction(ctx, builtins, "__rem", { ValueType::Any, ValueType::Any }, ValueType::Any,
        +[](RuntimeContext* ctx_, JSValue a, JSValue b, JSValue* res) {
            *res = quickjs_ops::rem(ctx_->ctx, a, b);
        }
    );

    return builtins;
}


}  // namespace jac::cfg::mir_emit
