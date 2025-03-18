#pragma once

#include <string>
#include <unordered_map>
#include <nlohmann/json.hpp>
#include <dlfcn.h>

class PluginInterface;

class PluginRegistry {
    using CreateFunc = PluginInterface* (*)(const std::string&);  // Function pointer for creating a plugin instance
    struct PluginRecord {
        CreateFunc createFunc;  // The function to create an instance
        void* handle; // Tracks the handle for dynamically loaded plugins (if needed)
        // Map instance ID to shared pointer for each active instance.
        std::unordered_map<std::string, std::weak_ptr<PluginInterface>> instances;
        // Which interfaces are implemented by the plugin (PluginExecutorInterface, PluginRunnableInterface, PluginInterface for example.
        std::vector<std::string> interfaces_implemented; // Pass through in the constructor of the plugin?
    };
public:
    static PluginRegistry* getInstance();

    std::vector<std::string> listAvailablePlugins();

    nlohmann::json listImplementedInterfaces(const std::string &pluginName);

    nlohmann::json listInstances();
    std::vector<std::string> listInstancesByCreateFunc(const std::string&);
     std::optional<std::string> getCreateFuncForInstance(const std::string &) const;

    bool loadAllPlugins();

    // Plugin CreateFunc & Instance Management
    void registerPlugin(const std::string&, CreateFunc);
    std::optional<std::pair<std::string, std::shared_ptr<PluginInterface>>> createPluginInstance(const std::string &pluginCreateFuncName, const nlohmann::json &options);
    bool unloadPluginHandle(const std::string &pluginCreateFuncName);
    bool insertInterfacesImplemented(const std::string &pluginName, const std::vector<std::string> &interfaces);

private:
    PluginRegistry() = default;
    std::unordered_map<std::string, PluginRecord> pluginRecords_;
    bool loadPluginCreateFunc(const std::string &);
};

struct PluginProxy {
    PluginRegistry* operator->() const {
        return PluginRegistry::getInstance();
    }
};

// ✅ Define a global `Plugins` variable for easy access
static PluginProxy Plugins;