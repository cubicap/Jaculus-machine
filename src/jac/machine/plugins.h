#pragma once

#include <jac/machine/machine.h>

#include <memory>
#include <vector>


namespace jac {


/**
 * @brief A base class for all plugins
 */
class Plugin {
public:
    virtual ~Plugin() = default;
};


/**
 * @brief A handle which can be used to retrieve a plugin from a machine
 */
class PluginHandle {
    size_t _index;
public:
    PluginHandle(size_t index) : _index(index) {}

    size_t index() const {
        return _index;
    }

    PluginHandle& operator++() {
        ++_index;
        return *this;
    }

    PluginHandle operator++(int) {
        PluginHandle copy = *this;
        ++_index;
        return copy;
    }

    PluginHandle operator+(size_t offset) {
        return PluginHandle(_index + offset);
    }

    bool operator==(const PluginHandle& other) const = default;
};


/**
 * @brief An MFeature that allows for inserting plugins into the machine
 * and retrieving them by index.
 */
template<typename Next>
class PluginHolderFeature : public Next {
private:
    std::vector<std::unique_ptr<Plugin>> _plugins;
public:
    PluginHolderFeature() = default;

    PluginHandle holdPlugin(std::unique_ptr<Plugin> plugin) {
        _plugins.push_back(std::move(plugin));
        return _plugins.size() - 1;
    }

    template<typename Plugin_t = Plugin>
    Plugin_t& getPlugin(PluginHandle handle) {
        return dynamic_cast<Plugin_t&>(*_plugins[handle.index()]);
    }

    PluginHandle pluginBegin() {
        return 0;
    }

    PluginHandle pluginEnd() {
        return _plugins.size();
    }
};


/**
 * @brief A class for managing groups of plugins and initializing them
 * all at once.
 */
class PluginManager {
private:
    std::vector<std::unique_ptr<Plugin>(*)(MachineBase&)> _plugins;
public:
    PluginManager() = default;


    /**
     * @brief Adds a plugin to the manager
     * @note The initialization of the plugin is to be performed by the constructor.
     * @tparam Plugin_t The type of the plugin to add
     * @return The offset of the plugin in the group. Can be added to the PluginHandle
     * returned by initialize() to get the handle to the plugin.
     */
    template<typename Plugin_t>
    size_t addPlugin() {
        _plugins.push_back(+[](MachineBase& machine) -> std::unique_ptr<Plugin> {
            return std::make_unique<Plugin_t>(machine);
        });

        return _plugins.size() - 1;
    }

    /**
     * @brief Initializes all plugins in the manager
     * @tparam Machine The type of the machine to initialize the plugins for
     * @param machine The machine to initialize the plugins for
     * @return A Handle to the first plugin initialized.
     */
    template<typename Machine>
    PluginHandle initialize(Machine& machine) {
        PluginHandle first = machine.pluginBegin();

        for (auto& plugin : _plugins) {
            machine.holdPlugin(plugin(machine));
        }

        return first;
    }
};


} // namespace jac
