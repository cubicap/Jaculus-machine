# Event loop

Although JavaScript programs are event-driven, the code is executed in a single thread. This is achieved by using an event loop, where asynchronous
events are queued and executed in the order they are received.

The default implementation of an event loop is provided by `EventLoopFeature`. It is an MFeature that can be added to a Machine to provide
an event loop and requires an MFeature that provides an event queue. The default event queue implementation is provided by `EventQueueFeature`.
To allow running code on an event loop tick, a virtual method `EventLoopFeature::onEventLoop` can be used. To define the top of the Machine stack,
the `EventLoopTerminal` class must be used as the topmost MFeature.


## Usage

To use the event loop, the MFeature must be added to the Machine. To start the event loop, use the `EventLoopFeature::run` method.

```cpp
#include <jac/machine.h>
#include <jac/features/eventLoopFeature.h>
#include <jac/features/eventQueueFeature.h>
#include <jac/features/timersFeature.h>

using Machine =
    EventLoopTerminal<
    TimersFeature<
    EventLoopFeature<
    EventQueueFeature<
    jac::MachineBase
>>>>;

Machine machine;
machine.initialize();

machine.eval("setTimeout(() => console.log('Hello, world!'), 1000);", "<stdin>", jac::EvalFlags::Global);
machine.startEventLoop();
```

The event loop will run indefinitely until the `EventLoopFeature::exit` or `EventLoopFeature::kill` method is called. The `exit` method can
also be called from the JavaScript code by calling the `exit` function.


## Custom event queue

It is possible to implement a custom event queue MFeature with extended functionality. The MFeature must be thread-safe and must provide the following methods:

```cpp
template<class Next>
class MyEventQueueFeature : public Next {
public:
    /**
     * @brief Check the event queue and return the first event
     * @param wait Wait for event if no event is available
     * @return Event or std::nullopt if no event is available
     */
    std::optional<std::function<void()>> getEvent(bool wait);

    /**
     * @brief Schedule an event to be run
     * @param func Function to be run
     */
    void scheduleEvent(std::function<void()> func);

    /**
     * @brief Wake up event loop if it is waiting for events
     */
    void notifyEventLoop();
};
```
