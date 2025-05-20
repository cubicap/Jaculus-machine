#include "mirUtil.h"
#include "quickjsOps.h"


namespace jac::cfg::mir_emit {


int getId() {
    static int counter = 0;
    return ++counter;
}


Builtins generateBuiltins(MIR_context_t ctx, RuntimeContext* rtCtx) {
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

    static constexpr auto getMemberCStr = [](RuntimeContext* ctx_, JSValue obj, const char* id, JSValue* res) {
        *res = JS_GetPropertyStr(ctx_->ctx, obj, id);
        if (JS_IsException(*res)) {
            ctx_->exceptionFlag = 1;
        }
    };
    static constexpr auto getMemberI32 = [](RuntimeContext* ctx_, JSValue obj, int32_t id, JSValue* res) {
        *res = JS_GetPropertyUint32(ctx_->ctx, obj, id);
        if (JS_IsException(*res)) {
            ctx_->exceptionFlag = 1;
        }
    };
    static constexpr auto getMemberAny = [](RuntimeContext* ctx_, JSValue obj, JSValue id, JSValue* res) {
        JSAtom atom = JS_ValueToAtom(ctx_->ctx, id);
        if (atom == JS_ATOM_NULL) {
            ctx_->exceptionFlag = 1;
            return;
        }
        *res = JS_GetProperty(ctx_->ctx, obj, atom);
        if (JS_IsException(*res)) {
            ctx_->exceptionFlag = 1;
        }
        JS_FreeAtom(ctx_->ctx, atom);
    };
    static constexpr auto setMemberCStr = [](RuntimeContext* ctx_, JSValue obj, const char* id, JSValue val) {
        if (JS_SetPropertyStr(ctx_->ctx, obj, id, val) < 0) {
            ctx_->exceptionFlag = 1;
        }
    };
    static constexpr auto setMemberI32 = [](RuntimeContext* ctx_, JSValue obj, int32_t id, JSValue val) {
        if (JS_SetPropertyUint32(ctx_->ctx, obj, id, val) < 0) {
            ctx_->exceptionFlag = 1;
        }
    };
    static constexpr auto setMemberAny = [](RuntimeContext* ctx_, JSValue obj, JSValue id, JSValue val) {
        JSAtom atom = JS_ValueToAtom(ctx_->ctx, id);
        if (atom == JS_ATOM_NULL) {
            ctx_->exceptionFlag = 1;
            return;
        }
        if (JS_SetProperty(ctx_->ctx, obj, atom, val) < 0) {
            ctx_->exceptionFlag = 1;
        }
        JS_FreeAtom(ctx_->ctx, atom);
    };
    addNativeFunction(ctx, builtins, "__getMemberObjCStr", { ValueType::Object, ValueType::StringConst }, ValueType::Any, true,
        +[](RuntimeContext* ctx_, JSObject* obj, const char* name, JSValue* res) {
            getMemberCStr(ctx_, JS_MKPTR(JS_TAG_OBJECT, obj), name, res);
        }
    );
    addNativeFunction(ctx, builtins, "__getMemberObjI32", { ValueType::Object, ValueType::I32 }, ValueType::Any, true,
        +[](RuntimeContext* ctx_, JSObject* obj, int32_t index, JSValue* res) {
            getMemberI32(ctx_, JS_MKPTR(JS_TAG_OBJECT, obj), index, res);
        }
    );
    addNativeFunction(ctx, builtins, "__getMemberObjAny", { ValueType::Object, ValueType::Any }, ValueType::Any, true,
        +[](RuntimeContext* ctx_, JSObject* obj, JSValue id, JSValue* res) {
            getMemberAny(ctx_, JS_MKPTR(JS_TAG_OBJECT, obj), id, res);
        }
    );
    addNativeFunction(ctx, builtins, "__getMemberAnyCStr", { ValueType::Any, ValueType::StringConst }, ValueType::Any, true,
        +[](RuntimeContext* ctx_, JSValue obj, const char* name, JSValue* res) {
            getMemberCStr(ctx_, obj, name, res);
        }
    );
    addNativeFunction(ctx, builtins, "__getMemberAnyI32", { ValueType::Any, ValueType::I32 }, ValueType::Any, true,
        +[](RuntimeContext* ctx_, JSValue obj, int32_t index, JSValue* res) {
            getMemberI32(ctx_, obj, index, res);
        }
    );
    addNativeFunction(ctx, builtins, "__getMemberAnyAny", { ValueType::Any, ValueType::Any }, ValueType::Any, true,
        +[](RuntimeContext* ctx_, JSValue obj, JSValue id, JSValue* res) {
            getMemberAny(ctx_, obj, id, res);
        }
    );

    addNativeFunction(ctx, builtins, "__setMemberObjCStr", { ValueType::Object, ValueType::StringConst, ValueType::Any }, ValueType::Void, true,
        +[](RuntimeContext* ctx_, JSObject* obj, const char* name, JSValue val) {
            setMemberCStr(ctx_, JS_MKPTR(JS_TAG_OBJECT, obj), name, val);
        }
    );
    addNativeFunction(ctx, builtins, "__setMemberObjI32", { ValueType::Object, ValueType::I32, ValueType::Any }, ValueType::Void, true,
        +[](RuntimeContext* ctx_, JSObject* obj, int32_t index, JSValue val) {
            setMemberI32(ctx_, JS_MKPTR(JS_TAG_OBJECT, obj), index, val);
        }
    );
    addNativeFunction(ctx, builtins, "__setMemberObjAny", { ValueType::Object, ValueType::Any, ValueType::Any }, ValueType::Void, true,
        +[](RuntimeContext* ctx_, JSObject* obj, JSValue id, JSValue val) {
            setMemberAny(ctx_, JS_MKPTR(JS_TAG_OBJECT, obj), id, val);
        }
    );
    addNativeFunction(ctx, builtins, "__setMemberAnyCStr", { ValueType::Any, ValueType::StringConst, ValueType::Any }, ValueType::Void, true,
        +[](RuntimeContext* ctx_, JSValue obj, const char* name, JSValue val) {
            setMemberCStr(ctx_, obj, name, val);
        }
    );
    addNativeFunction(ctx, builtins, "__setMemberAnyI32", { ValueType::Any, ValueType::I32, ValueType::Any }, ValueType::Void, true,
        +[](RuntimeContext* ctx_, JSValue obj, int32_t index, JSValue val) {
            setMemberI32(ctx_, obj, index, val);
        }
    );
    addNativeFunction(ctx, builtins, "__setMemberAnyAny", { ValueType::Any, ValueType::Any, ValueType::Any }, ValueType::Void, true,
        +[](RuntimeContext* ctx_, JSValue obj, JSValue id, JSValue val) {
            setMemberAny(ctx_, obj, id, val);
        }
    );

    static constexpr auto callAnyAny = [](RuntimeContext* ctx_, JSValue obj, JSValue this_, int32_t argc, JSValue* argv) {
        JSValue res = JS_Call(ctx_->ctx, obj, this_, argc, argv);
        if (JS_IsException(res)) {
            ctx_->exceptionFlag = 1;
            return;
        }
        argv[0] = res;
    };

    addNativeFunction(ctx, builtins, "__callAnyAny", { ValueType::Any, ValueType::Any, ValueType::I32 }, ValueType::Any, true, +callAnyAny);
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

    static constexpr auto callCtorAny = [](RuntimeContext* ctx_, JSValue obj, int32_t argc, JSValue* argv) {
        JSValue res = JS_CallConstructor(ctx_->ctx, obj, argc, argv);
        if (JS_IsException(res)) {
            ctx_->exceptionFlag = 1;
            return;
        }
        argv[0] = res;
    };

    addNativeFunction(ctx, builtins, "__callCtorAny", { ValueType::Any, ValueType::I32 }, ValueType::Any, true, +callCtorAny);
    addNativeFunction(ctx, builtins, "__callCtorObj", { ValueType::Object, ValueType::I32 }, ValueType::Any, true,
        +[](RuntimeContext* ctx_, JSObject* obj, int32_t argc, JSValue* argv) {
            callCtorAny(ctx_, JS_MKPTR(JS_TAG_OBJECT, obj), argc, argv);
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
                default: assert(false && "Invalid error type");
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

    addNativeFunction(ctx, builtins, "__remF64", { ValueType::F64, ValueType::F64 }, ValueType::F64, false,
        +[](RuntimeContext* ctx_, double a, double b) -> double {
            return std::fmod(a, b);
        }
    );

    addNativeFunction(ctx, builtins, "__boolConv", { ValueType::Any }, ValueType::Bool, true,
        +[](RuntimeContext* ctx_, JSValue a) -> int32_t {
            int res = JS_ToBool(ctx_->ctx, a);
            if (res < 0) {
                ctx_->exceptionFlag = 1;
                return 0;
            }
            return res;
        }
    );

    addNativeFunction(ctx, builtins, "__lessAny", { ValueType::Any, ValueType::Any }, ValueType::Bool, true,
        +[](RuntimeContext* ctx_, JSValue a, JSValue b) -> int32_t {
            return quickjs_ops::less(ctx_->ctx, a, b, &ctx_->exceptionFlag);
        }
    );
    addNativeFunction(ctx, builtins, "__lessEqAny", { ValueType::Any, ValueType::Any }, ValueType::Bool, true,
        +[](RuntimeContext* ctx_, JSValue a, JSValue b) -> int32_t {
            return quickjs_ops::lessEq(ctx_->ctx, a, b, &ctx_->exceptionFlag);
        }
    );
    addNativeFunction(ctx, builtins, "__greaterAny", { ValueType::Any, ValueType::Any }, ValueType::Bool, true,
        +[](RuntimeContext* ctx_, JSValue a, JSValue b) -> int32_t {
            return quickjs_ops::greater(ctx_->ctx, a, b, &ctx_->exceptionFlag);
        }
    );
    addNativeFunction(ctx, builtins, "__greaterEqAny", { ValueType::Any, ValueType::Any }, ValueType::Bool, true,
        +[](RuntimeContext* ctx_, JSValue a, JSValue b) -> int32_t {
            return quickjs_ops::greaterEq(ctx_->ctx, a, b, &ctx_->exceptionFlag);
        }
    );
    addNativeFunction(ctx, builtins, "__eqAny", { ValueType::Any, ValueType::Any }, ValueType::Bool, true,
        +[](RuntimeContext* ctx_, JSValue a, JSValue b) -> int32_t {
            return quickjs_ops::equal(ctx_->ctx, a, b, &ctx_->exceptionFlag);
        }
    );
    addNativeFunction(ctx, builtins, "__neqAny", { ValueType::Any, ValueType::Any }, ValueType::Bool, true,
        +[](RuntimeContext* ctx_, JSValue a, JSValue b) -> int32_t {
            return !quickjs_ops::equal(ctx_->ctx, a, b, &ctx_->exceptionFlag);
        }
    );

    addNativeFunction(ctx, builtins, "__newString", { ValueType::StringConst }, ValueType::Any, true,
        +[](RuntimeContext* ctx_, const char* str, JSValue* res) {
            *res = JS_NewString(ctx_->ctx, str);
            if (JS_IsException(*res)) {
                ctx_->exceptionFlag = 1;
            }
        }
    );

    return builtins;
}


}  // namespace jac::cfg::mir_emit
