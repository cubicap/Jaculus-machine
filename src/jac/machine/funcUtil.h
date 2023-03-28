#pragma once

#include <quickjs.h>
#include <exception>
#include "values.h"

namespace jac {


template<typename Func>
inline JSValue propagateExceptions(ContextRef ctx, Func& f) noexcept {
    try {
        return f();
    }
    catch (Exception& e) {
        return e.throwJS();
    }
    catch (std::exception& e) {
        return Exception::create(ctx, Exception::Type::InternalError, e.what()).throwJS();
    }
    catch (...) {
        return Exception::create(ctx, Exception::Type::InternalError, "unknown error").throwJS();
    }
}

template<typename Func>
inline JSValue propagateExceptions(ContextRef ctx, Func&& f) noexcept {
    return propagateExceptions(ctx, f);
}

template<typename Func, typename Res, typename... Args>
inline JSValue processCallRaw(ContextRef ctx, JSValueConst this_val, int argc, JSValueConst* argv, Func& f) {
    std::tuple<Args...> args = convertArgs<Args...>(ctx, argv, argc, std::make_index_sequence<sizeof...(Args)>());

    if constexpr (std::is_same_v<Res, void>) {
        std::apply(f, args);
        return JS_UNDEFINED;
    }
    else {
        return Value::from(ctx, std::apply(f, args)).loot().second;
    }
}

template<typename Func, typename Res, typename... Args>
inline Value processCall(ContextRef ctx, ValueWeak this_val, std::vector<ValueWeak> argv, Func& f) {
    std::tuple<Args...> args = convertArgs<Args...>(ctx, argv, std::make_index_sequence<sizeof...(Args)>());

    if constexpr (std::is_same_v<Res, void>) {
        std::apply(f, args);
        return Value::undefined(ctx);
    }
    else {
        return Value::from(ctx, std::apply(f, args));
    }
}

template<typename Func, typename Res>
inline JSValue processCallVariadicRaw(ContextRef ctx, JSValueConst this_val, int argc, JSValueConst* argv, Func& f) {
    std::vector<ValueWeak> args;
    for (int i = 0; i < argc; i++) {
        args.push_back(ValueWeak(ctx, argv[i]));
    }

    if constexpr (std::is_same_v<Res, void>) {
        f(args);
        return JS_UNDEFINED;
    }
    else {
        return Value::from(ctx, f(args)).loot().second;
    }
}

template<typename Func, typename Res>
inline Value processCallVariadic(ContextRef ctx, ValueWeak this_val, std::vector<ValueWeak> argv, Func& f) {
    if constexpr (std::is_same_v<Res, void>) {
        f(argv);
        return Value::undefined(ctx);
    }
    else {
        return Value::from(ctx, f(argv));
    }
}

template<typename Func, typename Res, typename... Args>
inline Value processCallThis(ContextRef ctx, ValueWeak this_val, std::vector<ValueWeak> argv, Func& f) {
    std::tuple<Args...> args = convertArgs<Args...>(ctx, argv, std::make_index_sequence<sizeof...(Args)>());

    if constexpr (std::is_same_v<Res, void>) {
        std::apply(f, std::tuple_cat(std::make_tuple(ctx, this_val), args));
        return Value::undefined(ctx);
    }
    else {
        return Value::from(ctx, std::apply(f, std::tuple_cat(std::make_tuple(ctx, this_val), args)));
    }
}

template<typename Func, typename Res>
inline Value processCallThisVariadic(ContextRef ctx, ValueWeak this_val, std::vector<ValueWeak> argv, Func& f) {

    if constexpr (std::is_same_v<Res, void>) {
        f(ctx, this_val, argv);
        return Value::undefined(ctx);
    }
    else {
        return Value::from(ctx, f(ctx, this_val, argv));
    }
}

template<typename... Args, std::size_t... Is>
constexpr std::tuple<Args...> convertArgs([[maybe_unused]]ContextRef ctx, std::vector<ValueWeak> argv, std::index_sequence<Is...>) {
    if (argv.size() != sizeof...(Args)) {
        throw Exception::create(ctx, Exception::Type::TypeError, "invalid number of arguments");
    }

    return std::make_tuple(argv[Is].to<Args>()...);
}

template<typename... Args, std::size_t... Is>
constexpr std::tuple<Args...> convertArgs([[maybe_unused]]ContextRef ctx, JSValueConst* argv, int argc, std::index_sequence<Is...>) {
    if (argc != sizeof...(Args)) {
        throw Exception::create(ctx, Exception::Type::TypeError, "invalid number of arguments");
    }

    return std::make_tuple(ValueWeak(ctx, argv[Is]).to<Args>()...);
}


} // namespace jac
