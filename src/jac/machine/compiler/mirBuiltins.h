#pragma once

#include <memory>
#include <mir.h>
#include <quickjs.h>

#include <string>
#include <vector>

#include "mirUtil.h"


namespace jac::cfg::mir_emit {


struct RuntimeContext {
    JSContext* ctx;
    std::vector<std::vector<JSValue>> freeStackFrames;
    std::vector<std::unique_ptr<char[]>> stringConsts;
};


struct Builtins {
    RuntimeContext* rtCtx;
    MIR_item_t jj_jProto;  // JSValue JSValue -> JSValue
    MIR_item_t j_vProto;   // JSValue -> void
    MIR_item_t v_vProto;   // void -> void
    MIR_item_t p_vProto;   // ptr -> void
    MIR_item_t pp_jProto;  // ptr ptr -> JSValue
    MIR_item_t dupVal;
    MIR_item_t pushFreeVal;
    MIR_item_t dupObj;
    MIR_item_t pushFreeObj;
    MIR_item_t enterStackFrame;
    MIR_item_t exitStackFrame;
    MIR_item_t getMemberObjCStr;
    MIR_item_t setMemberObjCStr;
};



// JSValue JSvalue -> *JSValue
inline void insertCallJJ_J(MIR_context_t ctx, MIR_item_t fun, MIR_item_t proto, RuntimeContext* rtCtx, MIR_item_t callable, MIR_reg_t base, int aOff, int bOff, int resOff) {
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

    auto a = MIR_new_mem_op(ctx, static_cast<MIR_type_t>(MIR_T_BLK + 1), sizeof(JSValue), aAddr, 0, 0);
    auto b = MIR_new_mem_op(ctx, static_cast<MIR_type_t>(MIR_T_BLK + 1), sizeof(JSValue), bAddr, 0, 0);

    auto rtCtxOp = MIR_new_int_op(ctx, reinterpret_cast<int64_t>(rtCtx));  // NOLINT
    MIR_append_insn(ctx, fun, MIR_new_call_insn(ctx, 6, MIR_new_ref_op(ctx, proto), MIR_new_ref_op(ctx, callable), rtCtxOp, a, b, MIR_new_reg_op(ctx, resAddr)));
}

inline MIR_item_t insertProtoJJ_J(MIR_context_t ctx) {
    MIR_var_t blkTestArgs[] = {
        { .type = MIR_T_P, .name = "rtCtx" },
        { .type = static_cast<MIR_type_t>(MIR_T_BLK + 1), .name = "a", .size = sizeof(JSValue) },
        { .type = static_cast<MIR_type_t>(MIR_T_BLK + 1), .name = "b", .size = sizeof(JSValue) },
        { .type = MIR_T_P, .name = "res" }
    };

    return MIR_new_proto_arr(ctx, "JJJ_proto", 0, nullptr, 4, blkTestArgs);
}

// JSValue -> void
inline void insertCallJ_V(MIR_context_t ctx, MIR_item_t fun, MIR_item_t proto, RuntimeContext* rtCtx, MIR_item_t callable, MIR_reg_t base, int aOff) {
    MIR_func_t func = MIR_get_item_func(ctx, fun);

    std::string aName = "_addr" + std::to_string(getId());
    auto aAddr = MIR_new_func_reg(ctx, func, MIR_T_I64, aName.c_str());
    MIR_append_insn(ctx, fun, MIR_new_insn(ctx, MIR_ADD, MIR_new_reg_op(ctx, aAddr), MIR_new_reg_op(ctx, base), MIR_new_int_op(ctx, aOff * sizeof(JSValue))));

    auto a = MIR_new_mem_op(ctx, static_cast<MIR_type_t>(MIR_T_BLK + 1), sizeof(JSValue), aAddr, 0, 0);

    auto rtCtxOp = MIR_new_int_op(ctx, reinterpret_cast<int64_t>(rtCtx));  // NOLINT
    MIR_append_insn(ctx, fun, MIR_new_call_insn(ctx, 4, MIR_new_ref_op(ctx, proto), MIR_new_ref_op(ctx, callable), rtCtxOp, a));
}

inline MIR_item_t insertProtoJ_V(MIR_context_t ctx) {
    MIR_var_t blkTestArgs[] = {
        { .type = MIR_T_P, .name = "rtCtx" },
        { .type = static_cast<MIR_type_t>(MIR_T_BLK + 1), .name = "a", .size = sizeof(JSValue) }
    };

    return MIR_new_proto_arr(ctx, "JV_proto", 0, nullptr, 2, blkTestArgs);
}

// void -> void
inline void insertCallV_V(MIR_context_t ctx, MIR_item_t fun, MIR_item_t proto, RuntimeContext* rtCtx, MIR_item_t callable) {
    auto rtCtxOp = MIR_new_int_op(ctx, reinterpret_cast<int64_t>(rtCtx));  // NOLINT
    MIR_append_insn(ctx, fun, MIR_new_call_insn(ctx, 3, MIR_new_ref_op(ctx, proto), MIR_new_ref_op(ctx, callable), rtCtxOp));
}

inline MIR_item_t insertProtoV_V(MIR_context_t ctx) {
    MIR_var_t blkTestArgs[] = {
        { .type = MIR_T_P, .name = "rtCtx" }
    };

    return MIR_new_proto_arr(ctx, "VV_proto", 0, nullptr, 1, blkTestArgs);
}

// ptr -> void
inline void insertCallP_V(MIR_context_t ctx, MIR_item_t fun, MIR_item_t proto, RuntimeContext* rtCtx, MIR_item_t callable, MIR_reg_t ptr) {
    auto rtCtxOp = MIR_new_int_op(ctx, reinterpret_cast<int64_t>(rtCtx));  // NOLINT
    MIR_append_insn(ctx, fun, MIR_new_call_insn(ctx, 4, MIR_new_ref_op(ctx, proto), MIR_new_ref_op(ctx, callable), rtCtxOp, MIR_new_reg_op(ctx, ptr)));
}

inline MIR_item_t insertProtoP_V(MIR_context_t ctx) {
    MIR_var_t blkTestArgs[] = {
        { .type = MIR_T_P, .name = "rtCtx" },
        { .type = MIR_T_P, .name = "ptr" }
    };

    return MIR_new_proto_arr(ctx, "PV_proto", 0, nullptr, 2, blkTestArgs);
}

// ptr ptr -> *JSValue
inline void insertCallPP_J(MIR_context_t ctx, MIR_item_t fun, MIR_item_t proto, RuntimeContext* rtCtx, MIR_item_t callable, MIR_reg_t ptrA, MIR_reg_t ptrB, MIR_reg_t resBase, int resOff) {
    MIR_func_t func = MIR_get_item_func(ctx, fun);

    std::string resAddrName = "_addr" + std::to_string(getId());
    auto resAddr = MIR_new_func_reg(ctx, func, MIR_T_I64, resAddrName.c_str());
    MIR_append_insn(ctx, fun, MIR_new_insn(ctx, MIR_ADD, MIR_new_reg_op(ctx, resAddr), MIR_new_reg_op(ctx, resBase), MIR_new_int_op(ctx, resOff * sizeof(JSValue))));

    auto rtCtxOp = MIR_new_int_op(ctx, reinterpret_cast<int64_t>(rtCtx));  // NOLINT
    auto ptrAOp = MIR_new_reg_op(ctx, ptrA);
    auto ptrBOp = MIR_new_reg_op(ctx, ptrB);
    MIR_append_insn(ctx, fun, MIR_new_call_insn(ctx, 6, MIR_new_ref_op(ctx, proto), MIR_new_ref_op(ctx, callable), rtCtxOp, ptrAOp, ptrBOp, MIR_new_reg_op(ctx, resAddr)));
}

inline MIR_item_t insertProtoPP_J(MIR_context_t ctx) {
    MIR_var_t blkTestArgs[] = {
        { .type = MIR_T_P, .name = "rtCtx" },
        { .type = MIR_T_P, .name = "ptrA" },
        { .type = MIR_T_P, .name = "ptrB" },
        { .type = MIR_T_P, .name = "res" }
    };

    return MIR_new_proto_arr(ctx, "PPJ_proto", 0, nullptr, 4, blkTestArgs);
}


inline Builtins generateBuiltins(MIR_context_t ctx, RuntimeContext* rtCtx) {
    MIR_load_external(ctx, "__dupVal", reinterpret_cast<void*>(+[](RuntimeContext* ctx_, JSValue a) {  // NOLINT
        JS_DupValue(ctx_->ctx, a);
    }));
    MIR_load_external(ctx, "__pushFreeVal", reinterpret_cast<void*>(+[](RuntimeContext* ctx_, JSValue a) {  // NOLINT
        ctx_->freeStackFrames.back().push_back(a);
    }));
    MIR_load_external(ctx, "__dupObj", reinterpret_cast<void*>(+[](RuntimeContext* ctx_, JSObject* obj) {  // NOLINT
        JSValue val = JS_MKPTR(JS_TAG_OBJECT, obj);
        JS_DupValue(ctx_->ctx, val);
    }));
    MIR_load_external(ctx, "__pushFreeObj", reinterpret_cast<void*>(+[](RuntimeContext* ctx_, JSObject* obj) {  // NOLINT
        JSValue val = JS_MKPTR(JS_TAG_OBJECT, obj);
        ctx_->freeStackFrames.back().push_back(val);
    }));

    MIR_load_external(ctx, "__enterStackFrame", reinterpret_cast<void*>(+[](RuntimeContext* ctx_) {  // NOLINT
        ctx_->freeStackFrames.emplace_back();
    }));
    MIR_load_external(ctx, "__exitStackFrame", reinterpret_cast<void*>(+[](RuntimeContext* ctx_) {  // NOLINT
        for (auto& val : ctx_->freeStackFrames.back()) {
            JS_FreeValue(ctx_->ctx, val);
        }
        ctx_->freeStackFrames.pop_back();
    }));

    MIR_load_external(ctx, "__getMemberObjCStr", reinterpret_cast<void*>(+[](RuntimeContext* ctx_, JSObject* obj, const char* name, JSValue* res) {  // NOLINT
        JSValue val = JS_MKPTR(JS_TAG_OBJECT, obj);
        *res = JS_GetPropertyStr(ctx_->ctx, val, name);
    }));
    MIR_load_external(ctx, "__setMemberObjCStr", reinterpret_cast<void*>(+[](RuntimeContext* ctx_, JSObject* obj, const char* name, JSValue val) {  // NOLINT
        JSValue objVal = JS_MKPTR(JS_TAG_OBJECT, obj);
        JS_SetPropertyStr(ctx_->ctx, objVal, name, val);
    }));

    Builtins builtins{
        .rtCtx = rtCtx,
        .jj_jProto = insertProtoJJ_J(ctx),
        .j_vProto = insertProtoJ_V(ctx),
        .v_vProto = insertProtoV_V(ctx),
        .p_vProto = insertProtoP_V(ctx),
        .pp_jProto = insertProtoPP_J(ctx),
        .dupVal = MIR_new_import(ctx, "__dupVal"),
        .pushFreeVal = MIR_new_import(ctx, "__pushFreeVal"),
        .dupObj = MIR_new_import(ctx, "__dupObj"),
        .pushFreeObj = MIR_new_import(ctx, "__pushFreeObj"),
        .enterStackFrame = MIR_new_import(ctx, "__enterStackFrame"),
        .exitStackFrame = MIR_new_import(ctx, "__exitStackFrame"),
        .getMemberObjCStr = MIR_new_import(ctx, "__getMemberObjCStr"),
        .setMemberObjCStr = MIR_new_import(ctx, "__setMemberObjCStr"),
    };

    return builtins;
}


}  // namespace jac::cfg::mir_emit
