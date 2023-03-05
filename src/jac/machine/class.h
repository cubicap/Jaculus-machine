#pragma once

#include <quickjs.h>

#include <vector>
#include <variant>
#include <tuple>
#include <string>
#include <experimental/type_traits>

#include "machine.h"
#include "values.h"
#include "funcUtil.h"


namespace jac {


template<typename Sgn>
struct SgnUnwrap;

template<typename Res, typename... Args>
struct SgnUnwrap<Res(Args...)> {
    using ResType = Res;
    using ArgTypes = std::tuple<Args...>;

    template<class Class>
    SgnUnwrap(Res (Class::*)(Args...)) {}
    template<class Class>
    SgnUnwrap(Res (Class::*)(Args...) const) {}
};
template<class Class, typename Res, typename... Args>
SgnUnwrap(Res (Class::*)(Args...)) -> SgnUnwrap<Res(Args...)>;
template<class Class, typename Res, typename... Args>
SgnUnwrap(Res (Class::*)(Args...) const) -> SgnUnwrap<Res(Args...)>;


template<template <typename...> class Base, typename Derived>
struct is_base_of_template_impl {
    template<typename... Ts>
    static constexpr void is_callable(Base<Ts...>*);

    template<typename T>
    using is_callable_t = decltype(is_callable(std::declval<T*>()));

    template<template<class...> class A, class Void = void>
    struct check : std::false_type {};

    template<template<class...> class A>
    struct check<A, std::void_t<A<Derived>>> : std::true_type {};

    using value_t = check<is_callable_t>;
};

template<template <typename...> class Base, typename Derived>
struct is_base_of_template : is_base_of_template_impl<Base, Derived>::value_t {};

template<template <typename...> class Base, typename Derived>
using is_base_of_template_t = typename is_base_of_template<Base, Derived>::type;

template<template <typename...> class Base, typename Derived>
inline constexpr bool is_base_of_template_v = is_base_of_template<Base, Derived>::value;


// TODO: property flags
namespace ProtoBuilder {

    template<typename T>
    struct Opaque {
        using OpaqueType = T;
        static inline JSClassID classId;

        static T* constructOpaque(ContextRef ctx, std::vector<ValueConst> args) {
            throw Exception::create(ctx, Exception::Type::TypeError, "Class cannot be instantiated");
        }

        static void destroyOpaque(JSRuntime* rt, T* ptr) noexcept {
            delete ptr;
        }


        static T* getOpaque(ContextRef ctx, ValueConst this_val) {
            T* ptr = reinterpret_cast<T*>(JS_GetOpaque(this_val.getVal(), classId));
            if (!ptr) {
                throw Exception::create(ctx, Exception::Type::TypeError, "Invalid opaque data");
            }
            return ptr;
        }

        template<typename Sgn, Sgn member>
        static Value callMember(ContextRef ctx, ValueConst func_obj, ValueConst this_val, std::vector<ValueConst> argv) {
            const SgnUnwrap Unwrap_(member);

            return [&]<typename Res, typename... Args>(SgnUnwrap<Res(Args...)>) {
                auto f = [&](Args... args) -> Res {
                    T* ptr = reinterpret_cast<T*>(JS_GetOpaque(func_obj.getVal(), classId));
                    return (ptr->*member)(args...);
                };

                return processCall<decltype(f), Res, Args...>(ctx, this_val, argv, f);
            }(Unwrap_);
        }


        template<typename U, U(T::*member)>
        static void addPropMember(ContextRef ctx, Object proto, std::string name, int flags = JS_PROP_C_W_E) {
            using GetRaw = JSValue(*)(JSContext* ctx_, JSValueConst this_val);
            using SetRaw = JSValue(*)(JSContext* ctx_, JSValueConst this_val, JSValueConst val);

            GetRaw get = [](JSContext* ctx_, JSValueConst this_val) -> JSValue {
                T* ptr = reinterpret_cast<T*>(JS_GetOpaque(this_val, classId));
                return Value::from(ctx_, ptr->*member).loot().second;
            };
            SetRaw set = [](JSContext* ctx_, JSValueConst this_val, JSValueConst val) -> JSValue {
                T* ptr = reinterpret_cast<T*>(JS_GetOpaque(this_val, classId));
                ptr->*member = ValueConst(ctx_, val).to<U>();
                return JS_UNDEFINED;
            };

            JSValue getter = JS_NewCFunction2(ctx, reinterpret_cast<JSCFunction*>(get), std::string("get " + name).c_str(), 0, JS_CFUNC_getter, 0);
            JSValue setter = JS_NewCFunction2(ctx, reinterpret_cast<JSCFunction*>(set), std::string("set " + name).c_str(), 1, JS_CFUNC_setter, 0);

            Atom atom = Atom(ctx, JS_NewAtom(ctx, name.c_str()));
            JS_DefinePropertyGetSet(ctx, proto.getVal(), atom.get(), getter, setter, flags);
        }

        template<typename Sgn, Sgn member>
        static void addMethodMember(ContextRef ctx, Object proto, std::string name, int flags = JS_PROP_C_W_E) {
            using MethodRaw = JSValue(*)(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst *argv);

            const SgnUnwrap Unwrap_(member);

            [&]<typename Res, typename... Args>(SgnUnwrap<Res(Args...)>) {
                MethodRaw func = [](JSContext* ctx_, JSValueConst this_val, int argc, JSValueConst* argv) -> JSValue {
                    T* ptr = reinterpret_cast<T*>(JS_GetOpaque(this_val, classId));

                    auto f = [ptr](Args... args) -> Res {
                        return (ptr->*member)(args...);
                    };

                    return propagateExceptions(ctx_, [&]() -> JSValue {
                        return processCallRaw<decltype(f), Res, Args...>(ctx_, this_val, argc, argv, f);
                    });
                };

                JSValue funcVal = JS_NewCFunction(ctx, reinterpret_cast<JSCFunction*>(func), name.c_str(), 0);

                Atom atom = Atom(ctx, JS_NewAtom(ctx, name.c_str()));
                JS_DefinePropertyValue(ctx, proto.getVal(), atom.get(), funcVal, flags);
            }(Unwrap_);
        }

        template<typename Sgn, Sgn member>
        static void addMethodMemberVariadic(ContextRef ctx, Object proto, std::string name, int flags = JS_PROP_C_W_E) {
            using MethodRaw = JSValue(*)(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst *argv);

            const SgnUnwrap Unwrap_(member);

            [&]<typename Res, typename... Args>(SgnUnwrap<Res(Args...)>) {
                MethodRaw func = [](JSContext* ctx_, JSValueConst this_val, int argc, JSValueConst* argv) -> JSValue {
                    T* ptr = reinterpret_cast<T*>(JS_GetOpaque(this_val, classId));

                    auto f = [ptr](std::vector<ValueConst> args) -> Res {
                        return (ptr->*member)(args);
                    };

                    return propagateExceptions(ctx_, [&]() -> JSValue {
                        return processCallVariadicRaw<decltype(f), Res>(ctx_, this_val, argc, argv, f);
                    });
                };

                JSValue funcVal = JS_NewCFunction(ctx, reinterpret_cast<JSCFunction*>(func), name.c_str(), 0);

                Atom atom = Atom(ctx, JS_NewAtom(ctx, name.c_str()));
                JS_DefinePropertyValue(ctx, proto.getVal(), atom.get(), funcVal, flags);
            }(Unwrap_);
        }
    };

    struct Callable {
        static bool isConstructor() {
            return false;
        }

        static Value callFunction(ContextRef ctx, ValueConst func_obj, ValueConst this_val, std::vector<ValueConst> args) {
            throw Exception::create(ctx, Exception::Type::TypeError, "Class cannot be called as a function");
        }

        static Value callConstructor(ContextRef ctx, ValueConst func_obj, ValueConst target, std::vector<ValueConst> args) {
            throw Exception::create(ctx, Exception::Type::TypeError, "Class cannot be called as a constructor");
        }
    };

    struct Properties {
        static void addProperties(ContextRef ctx, Object proto) {}


        static void addProp(ContextRef ctx, Object proto, std::string name, Value value, int flags = JS_PROP_C_W_E) {
            Atom atom = Atom(ctx, JS_NewAtom(ctx, name.c_str()));
            JS_DefinePropertyValue(ctx, proto.getVal(), atom.get(), value.loot().second, flags);
        }
    };
}


template<class Builder>
class Class {
    static inline JSClassID classId;
    static inline JSClassDef classDef;
    static inline std::string className;
    static inline bool isConstructor;

    static JSValue constructor_impl(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst *argv) noexcept {
        return propagateExceptions(ctx, [&]() -> JSValue {
            Value proto = Value::undefined(ctx);
            if (JS_IsUndefined(this_val)) {
                proto = Value(ctx, JS_GetClassProto(ctx, classId));
            }
            else {
                proto = Value(ctx, JS_GetPropertyStr(ctx, this_val, "prototype"));
            }
            Value obj(ctx, JS_NewObjectProtoClass(ctx, proto.getVal(), classId));

            if constexpr (std::is_base_of_v<ProtoBuilder::Callable, Builder>) {
                JS_SetConstructorBit(ctx, obj.getVal(), isConstructor);
            }

            if constexpr (is_base_of_template_v<ProtoBuilder::Opaque, Builder>) {
                std::vector<ValueConst> args;
                for (int i = 0; i < argc; i++) {
                    args.push_back(ValueConst(ctx, argv[i]));
                }
                auto instance = Builder::constructOpaque(ctx, args);
                JS_SetOpaque(obj.getVal(), instance);
            }

            return obj.loot().second;
        });
    }

public:
    static void init(std::string name, bool isCtor = false) {
        if (classId != 0) {
            if (className != name || isConstructor != isCtor) {
                throw std::runtime_error("Class already initialized with different name or constructor flag");
            }
            return;
        }
        JS_NewClassID(&classId);

        className = name;
        isConstructor = isCtor;

        JSClassFinalizer* finalizer = nullptr;
        JSClassCall* call = nullptr;

        if constexpr (is_base_of_template_v<ProtoBuilder::Opaque, Builder>) {
            Builder::classId = classId;
            finalizer = [](JSRuntime* rt, JSValue val) noexcept {
                static_assert(noexcept(Builder::destroyOpaque(rt, static_cast<typename Builder::OpaqueType*>(nullptr))));
                Builder::destroyOpaque(rt, reinterpret_cast<typename Builder::OpaqueType*>(JS_GetOpaque(val, classId)));
            };
        }

        if constexpr (std::is_base_of_v<ProtoBuilder::Callable, Builder>) {
            call = [](JSContext* ctx, JSValueConst func_obj, JSValueConst this_val, int argc, JSValueConst* argv, int flags) noexcept -> JSValue {
                std::vector<ValueConst> args;
                args.reserve(argc);
                for (int i = 0; i < argc; i++) {
                    args.push_back(ValueConst(ctx, argv[i]));
                }

                return propagateExceptions(ctx, [&]() -> JSValue {
                    if (flags & JS_CALL_FLAG_CONSTRUCTOR) {
                        return Builder::callConstructor(ctx, ValueConst(ctx, func_obj), ValueConst(ctx, this_val), args).loot().second;
                    } else {
                        return Builder::callFunction(ctx, ValueConst(ctx, func_obj), ValueConst(ctx, this_val), args).loot().second;
                    }

                    return JS_UNDEFINED;
                });
            };
        }

        classDef = {
            .class_name = className.c_str(),
            .finalizer = finalizer,
            .gc_mark = nullptr,
            .call = call,
            .exotic = nullptr
        };
    }

    static void initContext(ContextRef ctx) {
        JSRuntime* rt = JS_GetRuntime(ctx);
        if (!JS_IsRegisteredClass(rt, classId)) {
            JS_NewClass(rt, classId, &classDef);
        }
        auto proto = Object::create(ctx);


        if constexpr (std::is_base_of_v<ProtoBuilder::Properties, Builder>) {
            Builder::addProperties(ctx, proto);
        }

        Function ctor(ctx, JS_NewCFunction2(ctx, constructor_impl, className.c_str(), 0, JS_CFUNC_constructor, 0));
        JS_SetConstructor(ctx, ctor.getVal(), proto.getVal());

        JS_SetClassProto(ctx, classId, proto.loot().second);
    }

    static JSClassID getClassId() {
        return classId;
    }

    static Object getProto(ContextRef ctx) {
        JSRuntime* rt = JS_GetRuntime(ctx);
        if (!JS_IsRegisteredClass(rt, classId)) {
            JS_NewClass(rt, classId, &classDef);
        }
        Value proto = Value(ctx, JS_GetClassProto(ctx, classId));
        if (!JS_IsObject(proto.getVal())) {
            initContext(ctx);
            proto = Value(ctx, JS_GetClassProto(ctx, classId));
        }
        return proto.to<Object>();
    }

    /**
     * @brief Get the constructor object of this class in given context
     * @note if the class wasn't initialized in the context, it will be initialized
     *
     * @param ctx the context
     * @return the constructor object
     */
    static Function getConstructor(ContextRef ctx) {
        Object proto = getProto(ctx);
        return proto.get("constructor").to<Function>();
    }

    /**
     * @brief Create a new instance of this class in given context
     * @note if the class wasn't initialized in the context, it will be initialized
     *
     * @param ctx the context
     * @param instance a new-allocated instance to be saved as opaque data
     * @return the new instance
     */
    template<typename T, typename Bdr = Builder>
    static std::enable_if_t<is_base_of_template_v<ProtoBuilder::Opaque, Bdr>
            && std::is_base_of_v<typename Bdr::OpaqueType, T>
            && std::is_same_v<Bdr, Builder>, Value>
        createInstance(ContextRef ctx, T* instance) {
        // TODO: add createInstance for classes without opaque data
        Value proto = getProto(ctx);
        Value obj(ctx, JS_NewObjectProtoClass(ctx, proto.getVal(), classId));
        JS_SetOpaque(obj.getVal(), instance);
        JS_SetConstructorBit(ctx, obj.getVal(), isConstructor);
        return obj;
    }
};


} // namespace jac
