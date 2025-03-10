#pragma once


#include <iostream>
#include <mir.h>
#include <quickjs.h>

#include <string>

#include "mirUtil.h"


namespace jac::cfg::mir_emit {


struct RuntimeContext {
    JSContext* ctx;
};


struct Builtins {
    RuntimeContext* rtCtx;
    MIR_item_t jjjProto;
    MIR_item_t jvProto;
    MIR_item_t dup;
    MIR_item_t free;
};



// JSValue JSvalue -> *JSValue
inline void insertCallJJJ(MIR_context_t ctx, MIR_item_t fun, MIR_item_t proto, RuntimeContext* rtCtx, MIR_item_t callable, MIR_reg_t base, int aOff, int bOff, int resOff) {
    MIR_func_t func = MIR_get_item_func(ctx, fun);

    std::string aName = "_addr" + std::to_string(getId());
    std::string bName = "_addr" + std::to_string(getId());
    std::string resName = "_addr" + std::to_string(getId());

    auto aAddr = MIR_new_func_reg(ctx, func, MIR_T_I64, aName.c_str());
    auto bAddr = MIR_new_func_reg(ctx, func, MIR_T_I64, bName.c_str());
    auto resAddr = MIR_new_func_reg(ctx, func, MIR_T_I64, resName.c_str());

    MIR_append_insn(ctx, fun, MIR_new_insn(ctx, MIR_ADD, MIR_new_reg_op(ctx, aAddr), MIR_new_reg_op(ctx, base), MIR_new_int_op(ctx, aOff * sizeof(JSValue))));
    MIR_append_insn(ctx, fun, MIR_new_insn(ctx, MIR_ADD, MIR_new_reg_op(ctx, bAddr), MIR_new_reg_op(ctx, base), MIR_new_int_op(ctx, bOff * sizeof(JSValue))));
    MIR_append_insn(ctx, fun, MIR_new_insn(ctx, MIR_ADD, MIR_new_reg_op(ctx, resAddr), MIR_new_reg_op(ctx, base), MIR_new_int_op(ctx, resOff * sizeof(JSValue))));

    auto a = MIR_new_mem_op(ctx, MIR_type_t(MIR_T_BLK + 1), sizeof(JSValue), aAddr, 0, 0);
    auto b = MIR_new_mem_op(ctx, MIR_type_t(MIR_T_BLK + 1), sizeof(JSValue), bAddr, 0, 0);

    auto rtCtxOp = MIR_new_int_op(ctx, reinterpret_cast<int64_t>(rtCtx));  // NOLINT
    MIR_append_insn(ctx, fun, MIR_new_call_insn(ctx, 6, MIR_new_ref_op(ctx, proto), MIR_new_ref_op(ctx, callable), a, b, MIR_new_reg_op(ctx, resAddr)));
}

inline MIR_item_t insertProtoJJJ(MIR_context_t ctx) {
    MIR_var_t blkTestArgs[] = {
        { .type = MIR_T_P, .name = "rtCtx" },
        { .type = MIR_type_t(MIR_T_BLK + 1), .name = "a", .size = sizeof(JSValue) },
        { .type = MIR_type_t(MIR_T_BLK + 1), .name = "b", .size = sizeof(JSValue) },
        { .type = MIR_T_P, .name = "res" }
    };

    return MIR_new_proto_arr(ctx, "JJJ_proto", 0, nullptr, 4, blkTestArgs);
}

// JSValue -> void
inline void insertCallJV(MIR_context_t ctx, MIR_item_t fun, MIR_item_t proto, RuntimeContext* rtCtx, MIR_item_t callable, MIR_reg_t base, int aOff) {
    MIR_func_t func = MIR_get_item_func(ctx, fun);

    std::string aName = "_addr" + std::to_string(getId());
    auto aAddr = MIR_new_func_reg(ctx, func, MIR_T_I64, aName.c_str());
    MIR_append_insn(ctx, fun, MIR_new_insn(ctx, MIR_ADD, MIR_new_reg_op(ctx, aAddr), MIR_new_reg_op(ctx, base), MIR_new_int_op(ctx, aOff * sizeof(JSValue))));

    auto a = MIR_new_mem_op(ctx, MIR_type_t(MIR_T_BLK + 1), sizeof(JSValue), aAddr, 0, 0);

    auto rtCtxOp = MIR_new_int_op(ctx, reinterpret_cast<int64_t>(rtCtx));  // NOLINT
    MIR_append_insn(ctx, fun, MIR_new_call_insn(ctx, 4, MIR_new_ref_op(ctx, proto), MIR_new_ref_op(ctx, callable), rtCtxOp, a));
}

inline MIR_item_t insertProtoJV(MIR_context_t ctx) {
    MIR_var_t blkTestArgs[] = {
        { .type = MIR_T_P, .name = "rtCtx" },
        { .type = MIR_type_t(MIR_T_BLK + 1), .name = "a", .size = sizeof(JSValue) }
    };

    return MIR_new_proto_arr(ctx, "JV_proto", 0, nullptr, 2, blkTestArgs);
}


inline Builtins generateBuiltins(MIR_context_t ctx, RuntimeContext* rtCtx) {
    MIR_load_external(ctx, "__dup", reinterpret_cast<void*>(+[](RuntimeContext* ctx, JSValue a) {  // NOLINT
        std::cout << "dup " << ctx->ctx << " " << a.tag << " " << a.u.int32 << std::endl;
        JS_DupValue(ctx->ctx, a);
    }));
    MIR_load_external(ctx, "__free", reinterpret_cast<void*>(+[](RuntimeContext* ctx, JSValue a) {  // NOLINT
        std::cout << "free " << ctx->ctx << " " << a.tag << " " << a.u.int32 << std::endl;
        JS_FreeValue(ctx->ctx, a);
    }));

    Builtins builtins{
        .rtCtx = rtCtx,
        .jjjProto = insertProtoJJJ(ctx),
        .jvProto = insertProtoJV(ctx),
        .dup = MIR_new_import(ctx, "__dup"),
        .free = MIR_new_import(ctx, "__free"),
    };

    return builtins;
}


}  // namespace jac::cfg::mir_emit
