# Values

JavaScript values are represented using `ValueWrapper` class template and its subclasses. The template argument tells whether the value should be
freed upon the destruction of the wrapper. Thus, for convenience, there are type aliases like the following for all value types:

- `using Value = ValueWrapper<true>` - a wrapper for a value that should be freed (strong reference)
- `using ValueWeak = ValueWrapper<false>` - a wrapper for a value that should not be freed (weak reference)

`Value` has ownership of the value (reference), whereas `ValueWeak` does not, meaning the garbage collector can free the value referenced by a `ValueWeak`
at any time and render the wrapper invalid.

## Value types
- `Value` - plain value, can contain any type including `undefined` and `null`
- `Object`
- `Function`
- `Array`
- `Promise`
- `ArrayBuffer`
- `Exception` - either a JavaScript value or an Error template, which can be propagated to the JavaScript

API documentation for the value types can be found in the [API documentation](/doxygen/values_8h/#classes).


## Creating a JavaScript value
New values can be created using the static methods of the wrapper classes.

For example`Value::from<T>(ContextRef, T)` will convert the value to a JavaScript value (if a `ConvTraits` specialization exists for the type `T`)

```cpp
Value value = Value::from(machine.context(), 42);
```

A new object can be created using `Object::create(ContextRef)` method.

```cpp
Object obj = Object::create(machine.context());
```

## Converting a JavaScript value to a C++ type
Values can be converted to C++ types using `Value::to<T>()` method, which will convert the value to a C++ type.
There must exist a `ConvTraits` specialization for the type `T`.

```cpp
int i = value.to<int>();
```

Note that this conversion can be used to convert the value to a `Value` subclass, such as `Object`, `Function`, or `Exception`.

A list of default conversion traits can be found in [traits.h](/doxygen/traits_8h)


## Defining a custom conversion trait
To define a conversion trait for a custom type, simply define a specialization of the `ConvTraits` template for the type.

```cpp
template<>
struct ConvTraits<MyType> {
    static MyType from(ContextRef ctx, ValueWeak val) {
        // ...
    }

    static Value to(ContextRef ctx, MyType value) {
        // ...
    }
};
```
