#pragma once

#include "quickjs.h"

#include <cmath>
#include <string_view>


namespace jac::quickjs_ops {


inline JSValue add(JSContext* ctx, JSValue op1, JSValue op2) {
    JSValue res;

    if (JS_VALUE_IS_BOTH_INT(op1, op2)) {
        int64_t r;
        r = (int64_t)JS_VALUE_GET_INT(op1) + JS_VALUE_GET_INT(op2);
        if ((int)r != r)
            goto add_slow;
        res = JS_NewInt32(ctx, r);
    } else if (JS_VALUE_IS_BOTH_FLOAT(op1, op2)) {
        res = __JS_NewFloat64(ctx, JS_VALUE_GET_FLOAT64(op1) +
                                    JS_VALUE_GET_FLOAT64(op2));
    } else {
    add_slow:
        constexpr std::string_view code("(a, b) => a + b");

        JSValue args[2] = { op1, op2 };
        JSValue fn = JS_Eval(ctx, code.data(), code.size(), "<builtin_add>", JS_EVAL_TYPE_GLOBAL);
        res = JS_Call(ctx, fn, JS_UNDEFINED, 2, args);
        JS_FreeValue(ctx, fn);
    }

    return res;
}


enum class ArithOpcode {
    Sub,
    Mul,
    Div,
    Mod
};

static int js_binary_arith_slow(JSContext *ctx, JSValue op1, JSValue op2, JSValue* res, ArithOpcode op) {
    double d1, d2, r;

    if (JS_ToFloat64(ctx, &d1, op1)) {
        JS_FreeValue(ctx, op2);
        goto exception;
    }
    if (JS_ToFloat64(ctx, &d2, op2)) {
        goto exception;
    }
    switch(op) {
    case ArithOpcode::Sub:
        r = d1 - d2;
        break;
    case ArithOpcode::Mul:
        r = d1 * d2;
        break;
    case ArithOpcode::Div:
        r = d1 / d2;
        break;
    case ArithOpcode::Mod:
        r = fmod(d1, d2);
        break;
    default:
        abort();
    }
    *res = JS_NewFloat64(ctx, r);
    return 0;
 exception:
    return -1;
}


inline JSValue sub(JSContext* ctx, JSValue op1, JSValue op2) {
    JSValue res;

    if (JS_VALUE_IS_BOTH_INT(op1, op2)) {
        int64_t r;
        r = (int64_t)JS_VALUE_GET_INT(op1) - JS_VALUE_GET_INT(op2);
        if ((int)r != r)
            goto binary_arith_slow;
        res = JS_NewInt32(ctx, r);
    } else if (JS_VALUE_IS_BOTH_FLOAT(op1, op2)) {
        res = __JS_NewFloat64(ctx, JS_VALUE_GET_FLOAT64(op1) -
                                   JS_VALUE_GET_FLOAT64(op2));
    } else {
    binary_arith_slow:
        int ok = js_binary_arith_slow(ctx, op1, op2, &res, ArithOpcode::Sub);
        // TODO: handle error
    }
    return res;
}

inline JSValue mul(JSContext* ctx, JSValue op1, JSValue op2) {
    JSValue res;

    double d;
    if (JS_VALUE_IS_BOTH_INT(op1, op2)) {
        int32_t v1, v2;
        int64_t r;
        v1 = JS_VALUE_GET_INT(op1);
        v2 = JS_VALUE_GET_INT(op2);
        r = (int64_t)v1 * v2;
        if ((int)r != r) {
            d = (double)r;
            goto mul_fp_res;
        }
        /* need to test zero case for -0 result */
        if (r == 0 && (v1 | v2) < 0) {
            d = -0.0;
            goto mul_fp_res;
        }
        res = JS_NewInt32(ctx, r);
    } else if (JS_VALUE_IS_BOTH_FLOAT(op1, op2)) {
        d = JS_VALUE_GET_FLOAT64(op1) * JS_VALUE_GET_FLOAT64(op2);
    mul_fp_res:
        res = __JS_NewFloat64(ctx, d);
    } else {
        int ok = js_binary_arith_slow(ctx, op1, op2, &res, ArithOpcode::Mul);
        // TODO: handle error
    }
    return res;
}

inline JSValue div(JSContext* ctx, JSValue op1, JSValue op2) {
    JSValue res;

    if (JS_VALUE_IS_BOTH_INT(op1, op2)) {
        int v1, v2;
        v1 = JS_VALUE_GET_INT(op1);
        v2 = JS_VALUE_GET_INT(op2);
        res = JS_NewFloat64(ctx, (double)v1 / (double)v2);
    } else {
        int ok = js_binary_arith_slow(ctx, op1, op2, &res, ArithOpcode::Div);
        // TODO: handle error
    }
    return res;
}

inline JSValue rem(JSContext* ctx, JSValue op1, JSValue op2) {
    JSValue res;

    if (JS_VALUE_IS_BOTH_INT(op1, op2)) {
        int v1, v2, r;
        v1 = JS_VALUE_GET_INT(op1);
        v2 = JS_VALUE_GET_INT(op2);
        /* We must avoid v2 = 0, v1 = INT32_MIN and v2 =
           -1 and the cases where the result is -0. */
        if (v1 < 0 || v2 <= 0)
            goto binary_arith_slow;
        r = v1 % v2;
        res = JS_NewInt32(ctx, r);
    } else {
    binary_arith_slow:
        int ok = js_binary_arith_slow(ctx, op1, op2, &res, ArithOpcode::Mod);
        // TODO: handle error
    }
    return res;
}


}  // namespace jac::quickjs_ops
