# Context

According to ECMAScript specification, JavaScript code is evaluated in a Realm, which defines the execution environment (e.g., global object and set of built-in objects). QuickJS uses a different term for this concept -- Context, which I have adopted in Jaculus-machine.

Jaculus-machine allows creating only one Context per Machine. This Context is created when the Machine is initialized and is accessible through the
`MachineBase::context` method as a `ContextRef` object.

`ContextRef` can be used to access the global object of the Context through the `getGlobalObject` method. It is also an argument to many functions working with
JavaScript values.

## Usage

```cpp
#include <jac/machine.h>

using Machine = jac::MachineBase;

Machine machine;
machine.initialize();

jac::ContextRef ctx = machine.context();
jac::Object global = ctx.getGlobalObject();
```
