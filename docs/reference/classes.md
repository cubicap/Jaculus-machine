# Classes

Classes, in the context of Jaculus-machine are, similarly to JavaScript classes, templates for creating objects. Unlike raw JavaScript classes, however,
classes defined in C++ can contain opaque C++ data, which can then be used through an interface defined in the class definition.

## Defining a class

To create a class, you must first define a `ProtoBuilder` struct. A `ProtoBuilder` describes the class's behavior and prototype through a **static**
interface. Features of the class are specified by inheriting from structs from the `jac::ProtoBuilder` namespace and overriding their **static** interfaces.
These structs contain a default implementation of their interface and some convenience functions for describing the class.

The following base structs are available (with all of their interface methods):

```cpp
using namespace jac;

struct MyBuilder : public ProtoBuilder::Properties {
    static void addProperties(ContextRef ctx, Object proto) {
        proto.defineProperty("foo", Value::from(ctx, 42));
    }
};

struct MyBuilder2 : public ProtoBuilder::Callable {
    static Value callFunction(ContextRef ctx, ValueWeak funcObj, ValueWeak thisVal, std::vector<ValueWeak> args) {
        return Value::from(ctx, static_cast<int>(args.size()));
    }

    // the default implementation - does not have to be overridden if not needed
    static Value callConstructor(ContextRef ctx, ValueWeak funcObj, ValueWeak target, std::vector<ValueWeak> args) {
        throw Exception::create(Exception::Type::TypeError, "Class cannot be called as a constructor");
    }
};

struct MyBuilder3 : public ProtoBuilder::Opaque<MyClass>, public ProtoBuilder::Properties {
    static MyClass* constructOpaque(ContextRef ctx, std::vector<ValueWeak> args) {
        return new MyClass();
    }

    // the default implementation - does not have to be overridden if not needed
    static void destructOpaque(JSRuntime* rt, MyClass* ptr) {
        delete ptr;
    }

    static void addProperties(ContextRef ctx, Object proto) {
        addPropMember<int, &MyClass::foo>(ctx, proto, "foo");

        proto.defineProperty("bar", ff.newFunctionThis([](jac::ContextRef ctx, jac::ValueWeak thisValue) {
            // get the opaque pointer from the thisValue
            MyClass& self = *getOpaque(ctx, thisValue);
            self.bar();
        }));
    }
};
```

The `jac::Class` template can be instantiated with the `ProtoBuilder` struct to create a class definition and a name can be assigned using the `init` method.
The `init` method can be called repeatedly without any effect if called with the same name; otherwise, an exception will be thrown.

The following code shows some examples:

```cpp
using MyClass = Class<MyBuilder>;

MyClass::init("MyClass");
```

To use the class in a given Context, the Context must be initialized with the class definition:

```cpp
ContextRef ctx = ...;

MyClass::initContext(ctx);
```

After the class and context are initialized you can get the class constructor and prototype and instantiate the class:

```cpp
Value constructor = MyClass::getConstructor(ctx);
Value prototype = MyClass::getPrototype(ctx);

Value obj = constructor.to<Function>().callConstructor();
```
