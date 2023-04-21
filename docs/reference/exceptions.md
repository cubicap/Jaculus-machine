# Exceptions

In most cases, wrapped C++ code can throw exceptions, which will the be propagated to the JavaScript runtime.

Similarly, if any interaction with the JavaScript runtime throws an exception, it will be propagated to the C++ side.

## Exception propagation
When `jac::Exception` is thrown, the wrapped value is thrown or a new `Error` is constructed from given template.

When `std::exception` is thrown, an `InternalError` is thrown with the message `e.what()`.

When any other exception is thrown, an `InternalError` is thrown with the message `"unknown error"`.

If the JavaScript runtime throws an exception, a new `Exception` is constructed from the thrown value and thrown to the C++ side.


## `Exception` class
One way `Exception` can be used, is to wrap any JavaScript value. It can be created by converting a `Value` to `Exception`:

```cpp
Value value = ...;
Exception ex = value.to<Exception>();
```

Second way is to define `Exception` as an `Error` template. This way, the correct `Error` type will be constructed when
the exception is propagated to JavaScript:

```cpp
Exception ex = Exception::create(Exception::Type::TypeError, "Invalid argument");
```

More information can be found in the [API documentation](/doxygen/classjac_1_1ExceptionWrapper).
