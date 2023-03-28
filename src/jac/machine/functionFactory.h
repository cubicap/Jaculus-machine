#pragma once

#include "values.h"
#include "funcUtil.h"
#include "class.h"


namespace jac {


class FunctionFactory {
    /**
     * About exceptions propagation:
     *
     * When jac::Exception is thrown, the wrapped value or given error
     * type is thrown. When std::exception is thrown, an InternalError
     * is thrown. When any other exception is thrown, an InternalError
     * is thrown with the message "unknown error".
     */
    ContextRef _context;

    template<typename Func, typename Res, typename... Args>
    inline Function newFunctionHelper(Func& func, std::function<Res(Args...)>);

    template<typename Func, typename Res>
    inline Function newFunctionVariadicHelper(Func& func, std::function<Res(std::vector<ValueWeak>)>);

    template<typename Func, typename Res, typename... Args>
    inline Function newFunctionThisHelper(Func& func, std::function<Res(ContextRef, ValueWeak, Args...)>);

    template<typename Func, typename Res>
    inline Function newFunctionThisVariadicHelper(Func& func, std::function<Res(ContextRef, ValueWeak, std::vector<ValueWeak>)>);
public:
    FunctionFactory(ContextRef context) : _context(context) {}

    /**
     * @brief Wraps a C++ function into a javascript function object
     *
     * The expected signature of the function object is Res(Args...).
     * Arguments and return value of the function are automatically
     * converted to and from javascript values. Exceptions thrown within
     * the function are automatically propagated to the javascript side.
     *
     * @tparam Func type of the function to be wrapped
     * @param func the function object to be wrapped
     * @return The created function object
     */
    template<class Func>
    Function newFunction(Func func) {
        return newFunctionHelper(func, std::function(func));
    }

    /**
     * @brief Wraps a C++ function into a javascript function object
     *
     * The expected signature of the function object is
     * Res(std::vector<ValueWeak>). The vector will contain all
     * arguments passed to the function. The return value of the
     * function is automatically converted from a javascript value.
     * Exceptions thrown within the function are automatically
     * propagated to the javascript side.
     *
     * @tparam Func type of the function to be wrapped
     * @param func the function object to be wrapped
     * @return The created function object
     */
    template<class Func>
    Function newFunctionVariadic(Func func) {
        return newFunctionVariadicHelper(func, std::function(func));
    }

    template<class Func>
    Function newFunctionThis(Func func) {
        return newFunctionThisHelper(func, std::function(func));
    }

    template<class Func>
    Function newFunctionThisVariadic(Func func) {
        return newFunctionThisVariadicHelper(func, std::function(func));
    }
};


template<typename Func, typename Res, typename... Args>
inline Function FunctionFactory::newFunctionHelper(Func& func, std::function<Res(Args...)>) {
    Func* funcPtr = new Func(std::move(func));

    struct FuncProtoBuilder : public ProtoBuilder::Opaque<Func>, public ProtoBuilder::Callable {
        static Value callFunction(ContextRef ctx, ValueWeak func_obj, ValueWeak this_val, std::vector<ValueWeak> args) {
            Func* ptr = ProtoBuilder::Opaque<Func>::getOpaque(ctx, func_obj);
            return processCall<Func, Res, Args...>(ctx, this_val, args, *ptr);
        }
    };

    using FuncClass = Class<FuncProtoBuilder>;
    FuncClass::init("CppFunction");

    return static_cast<Value>(FuncClass::createInstance(_context, funcPtr)).to<Function>();
}

template<class Func, typename Res>
Function FunctionFactory::newFunctionVariadicHelper(Func& func, std::function<Res(std::vector<ValueWeak>)>) {
    Func* funcPtr = new Func(std::move(func));

    struct FuncProtoBuilder : public ProtoBuilder::Opaque<Func>, public ProtoBuilder::Callable {
        static Value callFunction(ContextRef ctx, ValueWeak func_obj, ValueWeak this_val, std::vector<ValueWeak> args) {
            Func* ptr = ProtoBuilder::Opaque<Func>::getOpaque(ctx, func_obj);
            return processCallVariadic<Func, Res>(ctx, this_val, args, *ptr);
        }
    };

    using FuncClass = Class<FuncProtoBuilder>;
    FuncClass::init("CppFunction");

    return static_cast<Value>(FuncClass::createInstance(_context, funcPtr)).to<Function>();
}

template<typename Func, typename Res, typename... Args>
Function FunctionFactory::newFunctionThisHelper(Func& func, std::function<Res(ContextRef, ValueWeak, Args...)>) {
    Func* funcPtr = new Func(std::move(func));

    struct FuncProtoBuilder : public ProtoBuilder::Opaque<Func>, public ProtoBuilder::Callable {
        static Value callFunction(ContextRef ctx, ValueWeak func_obj, ValueWeak this_val, std::vector<ValueWeak> args) {
            Func* ptr = ProtoBuilder::Opaque<Func>::getOpaque(ctx, func_obj);
            return processCallThis<Func, Res, Args...>(ctx, this_val, args, *ptr);
        }
    };

    using FuncClass = Class<FuncProtoBuilder>;
    FuncClass::init("CppFunction");

    return static_cast<Value>(FuncClass::createInstance(_context, funcPtr)).to<Function>();
}

template<typename Func, typename Res>
Function FunctionFactory::newFunctionThisVariadicHelper(Func& func, std::function<Res(ContextRef, ValueWeak, std::vector<ValueWeak>)>) {
    Func* funcPtr = new Func(std::move(func));

    struct FuncProtoBuilder : public ProtoBuilder::Opaque<Func>, public ProtoBuilder::Callable {
        static Value callFunction(ContextRef ctx, ValueWeak func_obj, ValueWeak this_val, std::vector<ValueWeak> args) {
            Func* ptr = ProtoBuilder::Opaque<Func>::getOpaque(ctx, func_obj);
            return processCallThisVariadic<Func, Res>(ctx, this_val, args, *ptr);
        }
    };

    using FuncClass = Class<FuncProtoBuilder>;
    FuncClass::init("CppFunction");

    return static_cast<Value>(FuncClass::createInstance(_context, funcPtr)).to<Function>();
}


} // namespace jac
