#pragma once

#include "Logger.hpp"
#include "PluginExecutorInterface.hpp"
#include "PluginRegistry.hpp"


class PluginInterface;
// RootPluginExecutor acts as the root of the execution tree, and executes only a single plugin.
// Typically, this would be an executor (such as SerialPluginExecutor), however it can be a plugin if needed.
using CreateFunc = PluginInterface* (*)();  // Function pointer for creating a plugin instance

class RootPluginExecutor final : public PluginExecutorInterface {
public:
    nlohmann::json printRecursiveInstanceTreeJson() override;
    bool instantiateRecursive() override;
    explicit RootPluginExecutor(const std::string& name);
    ~RootPluginExecutor() override = default;

    bool execute(EmailListView *) override;
    bool executeOne(EmailListView *, std::string instanceID) override;
    void clearAllInstances() override;
    std::shared_ptr<PluginInterface> getInstantiatedPluginByID(const std::string&) override;
    std::string getInstantiatedPluginConfig(const std::string& _) override;


    void updateInstantiatedPluginConfig(const std::string&, const nlohmann::json& ) override;
    void removeInstantiatedPlugin(const std::string& ) override;
    bool reloadPluginsFromConfig() override;

    std::string getInstantiatedPluginStatus(const std::string &) override;
    std::vector<std::string> listPluginsByID() override;

protected:
    std::string primaryPluginInstanceID_;
    std::shared_ptr<PluginInterface> primaryPlugin_;
private:
    struct Register {
        Register() {
            std::string name = "RootPluginExecutor";
            LOG_DEBUG_VERBOSE << "Registering plugin " << name;
            Plugins->registerPlugin(name, [](const std::string& instanceID) -> PluginInterface* {
                return new RootPluginExecutor(instanceID);
            });
        }
    };
    static inline Register reg_;  // Registers the schema when loaded
};