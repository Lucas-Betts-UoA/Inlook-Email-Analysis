#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <iostream>
#include <nlohmann/json.hpp>
#include <unordered_set>
#include <fstream>
#include <dlfcn.h>
#include <OrderedStringToPluginInterfaceMap.hpp>

#include "PluginExecutorInterface.hpp"
#include "PluginRegistry.hpp"

class SerialPluginExecutor final : public PluginExecutorInterface {
public:
    explicit SerialPluginExecutor(const std::string&);
    ~SerialPluginExecutor() override = default;

    bool execute(EmailListView * emailList) override;  // Executes all plugins

    bool instantiateRecursive() override;

    void clearAllInstances() override;
    nlohmann::json printRecursiveInstanceTreeJson() override;

    // Plugin Management (CRUD)
    std::shared_ptr<PluginInterface> getInstantiatedPluginByID(const std::string& pluginInstanceID) override;
    void updateInstantiatedPluginConfig(const std::string& pluginInstanceID, const nlohmann::json& newOptions) override;
    void removeInstantiatedPlugin(const std::string& pluginInstanceID) override;

    std::string getInstantiatedPluginConfig(const std::string& pluginInstanceID) override;
    bool reloadPluginsFromConfig() override;

    // State Management
    std::string getInstantiatedPluginStatus(const std::string &pluginInstanceID) override;
    std::vector<std::string> listPluginsByID() override;

    bool executeOne(EmailListView *, std::string) override;

private:
    struct Register {
        Register() {
            std::string name = "SerialPluginExecutor";
            LOG_DEBUG_VERBOSE << "Registering plugin " << name;
            Plugins->registerPlugin(name, [](const std::string& instanceID) -> PluginInterface* {
                return new SerialPluginExecutor(instanceID);
            });
        }
    };
    static inline Register reg;

    OrderedStringToPluginInterfaceMap managedPlugins_ = {}; // Ordered list of plugins
};
