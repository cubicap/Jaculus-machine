#pragma once

#include <cstdint>
#include <cstdlib>
#include <map>
#include <memory>
#include <ranges>
#include <string>
#include <vector>

#include <mir.h>
#include <quickjs.h>

#include "opcode.h"


namespace jac::cfg::mir_emit {


enum class ErrorType : int32_t {
    SyntaxError,
    TypeError,
    ReferenceError,
    RangeError,
    InternalError
};


int getId();


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


Builtins generateBuiltins(MIR_context_t ctx, RuntimeContext* rtCtx);


}  // namespace jac::cfg::mir_emit
