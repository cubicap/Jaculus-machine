#pragma once

#include <quickjs.h>
#include <cassert>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

#include "internal/declarations.h"
#include "context.h"
#include "stringView.h"
#include "atom.h"


namespace jac {


template<typename T>
T fromValue(ContextRef ctx, ValueConst val);

template<typename T>
Value toValue(ContextRef ctx, T val);


template<bool managed>
class ValueWrapper {
protected:
    ContextRef _ctx;
    JSValue _val;
public:
    ValueWrapper(ContextRef ctx, JSValue val) : _ctx(ctx), _val(val) {
        if (JS_IsException(_val)) {
            throw ctx.getException(*this);
        }
    }
    ValueWrapper(const ValueWrapper &other):
        _ctx(other._ctx),
        _val(managed ? JS_DupValue(_ctx, other._val) : other._val)
    {}
    ValueWrapper(ValueWrapper &&other) : _ctx(other._ctx), _val(other._val) {
        other._ctx = nullptr;
        other._val = JS_UNDEFINED;
    }

    ValueWrapper& operator=(const ValueWrapper &other) {
        if (managed) {
            if (_ctx) {
                JS_FreeValue(_ctx, _val);
            }
            _val = JS_DupValue(_ctx, other._val);
        }
        else {
            _val = other._val;
        }
        _ctx = other._ctx;
        return *this;
    }

    ValueWrapper& operator=(ValueWrapper &&other) {
        if (managed && _ctx) {
            JS_FreeValue(_ctx, _val);
        }
        _val = other._val;
        _ctx = other._ctx;
        other._val = JS_UNDEFINED;
        other._ctx = nullptr;
        return *this;
    }

    operator ValueWrapper<false>() {
        return ValueWrapper<false>(_ctx, _val);
    }

    ~ValueWrapper() {
        if (managed && _ctx) {
            JS_FreeValue(_ctx, _val);
        }
    }

    std::pair<ContextRef, JSValue> loot() {
        JSValue ret_ = _val;
        ContextRef ctx_ = this->_ctx;
        _ctx = nullptr;
        _val = JS_UNDEFINED;
        return {ctx_, ret_};
    }

    JSValue& getVal() {
        return _val;
    }

    bool isUndefined() {
        return JS_IsUndefined(_val);
    }

    bool isNull() {
        return JS_IsNull(_val);
    }

    bool isObject() {
        return JS_IsObject(_val);
    }

    bool isArray() {
        return JS_IsArray(_ctx, _val);
    }

    bool isFunction() {
        return JS_IsFunction(_ctx, _val);
    }

    StringView toString() {
        return to<StringView>();
    }

    template<typename T>
    T to() {
        return fromValue<T>(_ctx, *this);
    }

    ValueWrapper<true> stringify(int indent = 0) {
        auto idt = Value::from(_ctx, indent);
        return ValueWrapper(_ctx, JS_JSONStringify(_ctx, _val, JS_UNDEFINED, idt.getVal()));
    }

    template<typename T>
    static ValueWrapper<true> from(ContextRef ctx, T val) {
        return toValue(ctx, val);
    }

    static ValueWrapper<true> fromJSON(ContextRef ctx, std::string json, std::string filename = "<json>", bool extended = false) {
        return ValueWrapper(ctx, JS_ParseJSON2(ctx, json.c_str(), json.size(), filename.c_str(), extended ? JS_PARSE_JSON_EXT : 0));
    }

    static ValueWrapper<managed> undefined(ContextRef ctx) {
        return ValueWrapper(ctx, JS_UNDEFINED);
    }

    static ValueWrapper<managed> null(ContextRef ctx) {
        return ValueWrapper(ctx, JS_NULL);
    }

    friend std::ostream& operator<<(std::ostream& os, ValueWrapper& val) {
        os << val.toString();
        return os;
    }
};


template<bool managed>
class ExceptionWrapper : public ValueWrapper<managed>, public std::exception {
public:
    enum class Type {
        Any,
        Error,
        SyntaxError,
        TypeError,
        ReferenceError,
        RangeError,
        InternalError
    };
private:
    std::string _message;
    Type _type;
protected:
    using ValueWrapper<managed>::_val;
    using ValueWrapper<managed>::_ctx;
public:
    ExceptionWrapper(ValueWrapper<managed> value) : ValueWrapper<managed>(std::move(value)), _type(Type::Any) {
        _message = this->toString();
    }
    ExceptionWrapper(ContextRef ctx, JSValue val) : ExceptionWrapper(ValueWrapper<managed>(ctx, val)) {}

    std::string stackTrace() noexcept;

    const char* what() const noexcept override {
        return _message.c_str();
    }

    static ExceptionWrapper<true> create(ContextRef ctx, Type type, std::string message) {
        ExceptionWrapper<true> val = ValueWrapper<true>::null(ctx).to<ExceptionWrapper<true>>();
        val._type = type;
        val._message = message;
        return val;
    }


    JSValue throwJS();
};


template<bool managed>
class ObjectWrapper : public ValueWrapper<managed> {
protected:
    using ValueWrapper<managed>::_val;
    using ValueWrapper<managed>::_ctx;
public:
    ObjectWrapper(ValueWrapper<managed> value) : ValueWrapper<managed>(std::move(value)) {
        if (!this->isObject()) {
            throw Exception::create(_ctx, Exception::Type::TypeError, "not an object");
        }
    }
    ObjectWrapper(ContextRef ctx, JSValue val) : ObjectWrapper(ValueWrapper<managed>(ctx, val)) {}

    template<typename T = ValueWrapper<true>>
    T get(Atom prop) {
        ValueWrapper<true> val(_ctx, JS_GetProperty(_ctx, _val, prop.get()));
        return val.to<T>();
    }

    template<typename T = ValueWrapper<true>>
    T get(const std::string& name) {
        return get<T>(Atom::create(_ctx, name.c_str()));
    }

    template<typename T = ValueWrapper<true>>
    T get(uint32_t idx) {
        return get<T>(Atom::create(_ctx, idx));
    }

    template<typename T>
    void set(Atom prop, T val) {
        JS_SetProperty(_ctx, _val, prop.get(), toValue(_ctx, val).loot().second);
    }

    template<typename T>
    void set(const std::string& name, T val) {
        set(Atom::create(_ctx, name.c_str()), val);
    }

    template<typename T>
    void set(uint32_t idx, T val) {
        set(Atom::create(_ctx, idx), val);
    }

    template<typename Res, typename... Args>
    Res invoke(Atom prop, Args... args);

    template<typename Res, typename... Args>
    Res invoke(const std::string& key, Args... args) {
        return invoke<Res>(Atom::create(_ctx, key.c_str()), args...);
    }

    template<typename Res, typename... Args>
    Res invoke(uint32_t idx, Args... args) {
        return invoke<Res>(Atom::create(_ctx, idx), args...);
    }

    template<typename Id>
    void defineProperty(Id id, ValueWrapper<true> value, int flags = JS_PROP_C_W_E) {
        Atom atom = Atom::create(_ctx, id);
        JS_DefinePropertyValue(_ctx, _val, atom.get(), value.loot().second, flags);
    }

    template<typename Id>
    bool hasProperty(Id id) {
        Atom atom = Atom::create(_ctx, id);
        return JS_HasProperty(_ctx, _val, atom.get());
    }

    template<typename Id>
    void deleteProperty(Id id) {
        Atom atom = Atom::create(_ctx, id);
        JS_DeleteProperty(_ctx, _val, atom.get(), 0);
    }

    ObjectWrapper<true> getPrototype() {
        return ObjectWrapper<true>(_ctx, JS_GetPrototype(this->_ctx, this->_val));
    }

    void setPrototype(ObjectWrapper<true> proto) {
        JS_SetPrototype(this->_ctx, this->_val, proto.getVal());
    }

    static ObjectWrapper<true> create(ContextRef ctx) {
        return ObjectWrapper<true>(ctx, JS_NewObject(ctx));
    }
};


template<bool managed>
class FunctionWrapper : public ObjectWrapper<managed> {
protected:
    using ObjectWrapper<managed>::_val;
    using ObjectWrapper<managed>::_ctx;
public:
    FunctionWrapper(ObjectWrapper<managed> value) : ObjectWrapper<managed>(std::move(value)) {
        if (!this->isFunction()) {
            throw Exception::create(this->_ctx, Exception::Type::TypeError, "not a function");
        }
    }
    FunctionWrapper(ContextRef ctx, JSValue val) : FunctionWrapper(ObjectWrapper<managed>(ctx, val)) {}

    template<typename Res, typename... Args>
    Res callThis(ValueWrapper<false> thisVal, Args... args) {
        std::vector<JSValue> vals;
        vals.reserve(sizeof...(Args));
        try {
            (vals.push_back(toValue(_ctx, args).loot().second), ...);
            ValueWrapper<true> ret(_ctx, JS_Call(_ctx, _val, thisVal.getVal(), vals.size(), vals.data()));

            for (auto &v : vals) {
                JS_FreeValue(_ctx, v);
            }
            vals.clear();

            return ret.to<Res>();
        } catch (ExceptionWrapper<true> &e) {
            for (auto &v : vals) {
                JS_FreeValue(_ctx, v);
            }
            throw e;
        }
    }

    template<typename Res, typename... Args>
    Res call(Args... args) {
        return callThis<Res>(ValueWrapper<true>::undefined(_ctx), args...);
    }

    template<typename... Args>
    ValueWrapper<true> callConstructor(Args... args) {
        std::vector<JSValue> vals;
        vals.reserve(sizeof...(Args));
        try {
            (vals.push_back(toValue(_ctx, args).loot().second), ...);
            ValueWrapper<true> ret(_ctx, JS_CallConstructor(_ctx, _val, vals.size(), vals.data()));

            for (auto &v : vals) {
                JS_FreeValue(_ctx, v);
            }
            vals.clear();

            return ret;
        } catch (ExceptionWrapper<true> &e) {
            for (auto &v : vals) {
                JS_FreeValue(_ctx, v);
            }
            throw e;
        }
    }
};


template<bool managed>
class ArrayWrapper : public ObjectWrapper<managed> {
protected:
    using ObjectWrapper<managed>::_val;
    using ObjectWrapper<managed>::_ctx;
public:
    ArrayWrapper(ObjectWrapper<managed> value) : ObjectWrapper<managed>(std::move(value)) {
        if (!this->isArray()) {
            throw Exception::create(this->_ctx, Exception::Type::TypeError, "not an array");
        }
    }
    ArrayWrapper(ContextRef ctx, JSValue val) : ArrayWrapper(ObjectWrapper<managed>(ctx, val)) {}

    int length() {
        return this->template get<int>("length");
    }

    static ArrayWrapper<true> create(ContextRef ctx) {
        return ArrayWrapper<true>(ctx, JS_NewArray(ctx));
    }
};


template<bool managed>
class PromiseWrapper : public ObjectWrapper<managed> {
protected:
    using ObjectWrapper<managed>::_val;
    using ObjectWrapper<managed>::_ctx;
public:
    PromiseWrapper(ObjectWrapper<managed> value) : ObjectWrapper<managed>(std::move(value)) {
        // TODO: check if promise
    }
    PromiseWrapper(ContextRef ctx, JSValue val) : PromiseWrapper(ObjectWrapper<managed>(ctx, val)) {}

    /**
     * @brief Create a new Promise object
     * @return tuple of the Promise object, resolve function, reject function
     */
    static std::tuple<Promise, Function, Function> create(ContextRef ctx) {
        JSValue functions[2];
        JSValue promise = JS_NewPromiseCapability(ctx, functions);
        return std::make_tuple(Promise(ctx, promise), Function(ctx, functions[0]), Function(ctx, functions[1]));
    }
};


template<bool managed>
template<typename Res, typename... Args>
Res ObjectWrapper<managed>::invoke(Atom key, Args... args) {
    return get<FunctionWrapper<true>>(key).template callThis<Res>(*this, args...);
};


template<bool managed>
std::string ExceptionWrapper<managed>::stackTrace() noexcept {
    try {
        ObjectWrapper<false> obj(*this);
        return obj.get("stack").toString().c_str();
    } catch (std::exception &e) {
        return "failed to get stack trace: " + std::string(e.what());
    }
}


template<bool managed>
JSValue ExceptionWrapper<managed>::throwJS() {
    if (!_ctx) {
        throw std::runtime_error("no value to throw");
    }

    if (_type == Type::Any) {
        auto [ctx, val] = ValueWrapper<managed>::loot();
        return JS_Throw(ctx, val);
    }


    if (_type == Type::Error) {
        ObjectWrapper<false> errObj(_ctx, JS_NewError(_ctx));
        errObj.set("message", _message);

        return JS_Throw(_ctx, errObj.getVal());
    }

    switch (_type) {
        case Type::SyntaxError:
            return JS_ThrowSyntaxError(_ctx, "%s", _message.c_str());
        case Type::TypeError:
            return JS_ThrowTypeError(_ctx, "%s", _message.c_str());
        case Type::ReferenceError:
            return JS_ThrowReferenceError(_ctx, "%s", _message.c_str());
        case Type::RangeError:
            return JS_ThrowRangeError(_ctx, "%s", _message.c_str());
        case Type::InternalError:
            return JS_ThrowInternalError(_ctx, "%s", _message.c_str());
        default:
            return JS_Throw(_ctx, JS_NewError(_ctx));
    }
}


} // namespace jac


#include "traits.h"


namespace jac {


template<typename T>
T fromValue([[maybe_unused]] ContextRef ctx, [[maybe_unused]] ValueConst val) {
    if constexpr (std::is_same_v<T, void>) {
        return;
    }
    else {
        return ConvTraits<T>::from(ctx, val);
    }
}

template<typename T>
Value toValue([[maybe_unused]] ContextRef ctx, [[maybe_unused]] T value) {
    if constexpr (std::is_same_v<T, void>) {
        return Value::undefined(ctx);
    }

    auto val = ConvTraits<T>::to(ctx, value);

    return val;
}


}
