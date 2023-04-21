# Getting started

The core concept of Jaculus-machine is a *Machine* type. A *Machine* is parametrized by *Features* that should be part of it.
*Machine* is created using stack inheritance. At the bottom of the stack, there is always `jac::MachineBase` which provides the basic functionality.
For example, a *Machine* with core *Features* can be created like this:

```cpp
#include <jaculus/machine.h>
#include <jaculus/features/stdioFeature.h>
#include <jaculus/features/filesystemFeature.h>
#include <jaculus/features/basicStreamFeature.h>
#include <jaculus/features/moduleLoaderFeature.h>
#include <jaculus/features/util/ostreamjs.h>


using Machine =
    ModuleLoaderFeature<
    FilesystemFeature<
    StdioFeature<
    BasicStreamFeature<
    jac::MachineBase
>>>>;
```

After the machine is defined, we can create an instance of the machine:

```cpp
Machine machine;
```

Then we can configure the machine - we can, for example, set the output streams of stdio feature:

```cpp
machine.stdio.out = machine.stdio.out = std::make_unique<OsWritable<Machine>>(std::cout);
machine.stdio.err = machine.stdio.err = std::make_unique<OsWritable<Machine>>(std::cerr);
```

At this point, we still can not run any JavaScript code, nor can we interact with the runtime in
any way. For that, we first need to initialize the machine:

```cpp
machine.initialize();
```

After the machine is initialized, we can finally run some JavaScript code:

```cpp
jac::Value result = machine.eval("console.log('Hello, world!');", "<stdin>", jac::EvalFlags::Global);
```

Or we can evaluate a JavaScript file using `ModuleLoaderFeature::evalFile` method:

```cpp
jac::Value result = machine.evalFile("./main.js", jac::EvalFlags::Global);
```

Both `eval` and `evalFile`, however, only evaluate the code and do not control the event loop. For that, we would need `EventLoopFeature`.


## Features
*Features* are the building blocks of a *Machine*. The stack design of *Machine* allows interfacing with different *Features* in C++ directly
and to implement new *Features* on top of existing ones without a large performance penalty.

Only the most basic and necessary *Features* are included with the library:

- `jac::MachineBase` - the base class of all machines. It provides the basic functionality of the runtime.
- `EventQueueFeature` - provides an event queue built on top of the standard library.
- `EventLoopFeature` - provides a default event loop implementation.
- `FileystemFeature` - provides filesystem access through `fs` and `path` modules
- `ModuleLoaderFeature` - provides module loading through `import`, and `evalFile` method
- `BasicStreamFeature` - provides `Readable` and `Writable` abstract classes
- `StdioFeature` - provides stdio streams and `console` interface
- `TimersFeature` - provides typical JavaScript timers (with a slight difference) and a `sleep` function
