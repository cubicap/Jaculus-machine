# Custom Features

As shown in the [Getting started](getting-started.md) section, a *Machine* is parametrized by *Features* that provide functionality to the
*Machine* and the JavaScript runtime.

A *Feature* is a class, that follows the stack inheritance and provides functionality to the *Machine* either by providing a public C++ API
that can be used by other *Features* or the user, or by adding functionality directly to the JavaScript runtime.

The *Feature* must not interact with the JavaScript runtime in any way in its constructor, as it is not yet initialized at that point.
Initialization of the *Feature* involving the JavaScript runtime should be done in the `initialize` method.

## Modifying the global object

Let's create a *Feature* that adds a `print` function to the global object of the JavaScript runtime.

```cpp
#include <jac/machine.h>
#include <jac/functionFactory.h>

template<class Next>
class PrintFeature : public Next {
public:
    std::ostream out;

    void print(std::string str) {
        out << str;
    }

    void initialize() {
        Next::initialize();

        jac::Value global = this->context().getGlobalObject();
        jac::FunctionFactory ff(this->context());

        global.defineProperty("print", ff.newFunction([this](std::string str) {
            this->print(str);
        }));
    }
};
```

The feature lets the user set the output stream of the `print` function by setting the `out` field. The `initialize` method then adds the `print` function to the global object of the JavaScript runtime.

The feature can be used like this:

```cpp
using Machine = PrintFeature<jac::MachineBase>;

Machine machine;
machine.print.out = std::cout;

machine.initialize();

machine.eval("print('Hello, world!');", "<eval>", jac::EvalFlags::Global);
```

## Defining a JavaScript module

Often, a *Feature* will want to define a JavaScript module. This can be done by using the `MachineBase::newModule` method.

```cpp
#include <jac/machine.h>
#include <jac/functionFactory.h>

template<class Next>
class PrintFeature : public Next {
public:
    std::ostream out;

    void print(std::string str) {
        out << str;
    }

    void initialize() {
        Next::initialize();

        jac::FunctionFactory ff(this->context());

        jac::Module& mdl = this->newModule("printer");
        mdl.addExport("print", ff.newFunction([this](std::string str) {
            this->print(str);
        }));
    }
};
```
