# Plugins

Plugins are the primary way to extend the functionality of the JavaScript runtime. They are added to
the machine dynamically after the runtime initialization.

Compared to MFeatures, Plugins are more high-level and should be used to implement functionality that
is not directly related to the JavaScript runtime.

## Creating a Plugin

Plugins are created by subclassing the `Plugin` class. The `Plugin` initialization is done in the class
constructor, with the Machine passed as an argument. The Machine should be already initialized at this
point, so it is safe to interact with the JavaScript runtime.

As an example, let's use the `ReportPlugin` used in tests:

```cpp
class ReportPlugin : public jac::Plugin {
    std::vector<std::string> _reports;
public:
    void report(std::string report) {
        _reports.push_back(report);
    }

    const std::vector<std::string>& getReports() {
        return _reports;
    }
public:
    ReportPlugin(jac::MachineBase& machine) {
        jac::FunctionFactory ff(machine.context());
        jac::Object global = machine.context().getGlobalObject();

        global.defineProperty("report", ff.newFunction([this](jac::ValueWeak val) {
            this->report(val.to<std::string>());
        }));
    }
};
```


## Adding a Plugin to the Machine

First, we need to create a Machine, which, at the top of the stack, will have the `PluginHolderFeature`.

```cpp
using Machine = jac::ComposeMachine<
    jac::MachineBase,
    ...
    jac::PluginHolderFeature
>;
```

The typical way of adding a Plugin to the Machine is by using the `jac::PluginManager` class, which
can be used to add groups of Plugins to the Machine.

```cpp
jac::PluginManager pm;
auto offset = pm.addPlugin<ReportPlugin>();

Machine machine;
machine.initialize();

jac::PluginHandle handle = pm.initialize(machine);

machine.eval("report('Hello, world!');", "<eval>", jac::EvalFlags::Global);

ReportPlugin& rp = machine.getPlugin<ReportPlugin>(handle + offset);
assert(rp.getReports() == std::vector<std::string>{"Hello, world!"});
```

By adding the Plugin to the PluginManager, we get an offset of the Plugin in the group. Next, by using
the `PluginManager::initialize` method, we get the handle of the first plugin in the group. The handle
can be used to get the Plugin from the Machine by adding the offset to it.
