#pragma once
#include "PluginExecutorInterface.hpp"
#include "PluginRegistry.hpp"

class ParallelPluginExecutor final : public PluginExecutorInterface {
public:
    explicit ParallelPluginExecutor(const std::string&);
    ~ParallelPluginExecutor() override = default;

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
            std::string name = "ParallelPluginExecutor";
            LOG_DEBUG_VERBOSE << "Registering plugin " << name;
            Plugins->registerPlugin(name, [](const std::string& instanceID) -> PluginInterface* {
                return new ParallelPluginExecutor(instanceID);
            });
        }
    };

    std::unordered_map<std::string, std::shared_ptr<PluginInterface>> managedPlugins_;
    static inline Register reg;
};
