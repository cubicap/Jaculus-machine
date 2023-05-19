# Getting started

The core concept of Jaculus-machine is a Machine type. A Machine is parametrized by MFeatures that should be part of it.
Machine is created using stack inheritance. At the bottom of the stack, there is always `jac::MachineBase` which provides the basic functionality.
For example, a Machine with core MFeatures can be created like this:

```cpp
#include <jaculus/machine.h>
#include <jaculus/features/stdioFeature.h>
#include <jaculus/features/filesystemFeature.h>
#include <jaculus/features/basicStreamFeature.h>
#include <jaculus/features/moduleLoaderFeature.h>
#include <jaculus/features/util/ostreamjs.h>


using Machine = jac::ComposeMachine<
    jac::MachineBase
    jac::BasicStreamFeature,
    jac::StdioFeature,
    jac::FilesystemFeature,
    jac::ModuleLoaderFeature,
>;
```

After the Machine type is defined, we can create its instance:

```cpp
Machine machine;
```

Then we can configure the Machine - we can, for example, set the output streams of the stdio MFeature:

```cpp
machine.stdio.out = machine.stdio.out = std::make_unique<OsWritable<Machine>>(std::cout);
machine.stdio.err = machine.stdio.err = std::make_unique<OsWritable<Machine>>(std::cerr);
```

At this point, we still can not run any JavaScript code, nor can we interact with the runtime in
any way. For that, we first need to initialize the Machine:

```cpp
machine.initialize();
```

After the Machine is initialized, we can finally run some JavaScript code:

```cpp
jac::Value result = machine.eval("console.log('Hello, world!');", "<stdin>", jac::EvalFlags::Global);
```

Or we can evaluate a JavaScript file using `ModuleLoaderFeature::evalFile` method:

```cpp
jac::Value result = machine.evalFile("./main.js", jac::EvalFlags::Global);
```

Both `eval` and `evalFile`, however, only evaluate the code and do not control the event loop. For that, we would need `EventLoopFeature`.


## MFeatures
MFeatures are the building blocks of a Machine. The stack design of Machine allows interfacing with different MFeatures in C++ directly
and to implement new MFeatures on top of existing ones without a large performance penalty.

Only the most basic and necessary MFeatures are included with the library:

- `jac::MachineBase` - the base class of all Machines. It provides the basic functionality of the runtime.
- `jac::EventQueueFeature` - provides an event queue built on top of the standard library.
- `jac::EventLoopFeature` - provides a default event loop implementation.
- `jac::FileystemFeature` - provides filesystem access through `fs` and `path` modules
- `jac::ModuleLoaderFeature` - provides module loading through `import`, and `evalFile` method
- `jac::BasicStreamFeature` - provides `Readable` and `Writable` abstract classes
- `jac::StdioFeature` - provides stdio streams and `console` interface
- `jac::TimersFeature` - provides typical JavaScript timers (with a slight difference) and a `sleep` function
