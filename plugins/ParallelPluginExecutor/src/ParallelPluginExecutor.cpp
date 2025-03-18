#include <future>

#include "ParallelPluginExecutor.hpp"
#include "Logger.hpp"
#include "EmailListView.hpp"
#include "PluginInterface.hpp"
#include "PluginRegistry.hpp"

ParallelPluginExecutor::ParallelPluginExecutor(const std::string& instanceID) : PluginExecutorInterface(instanceID) {
    pluginName_ = "ParallelPluginExecutor";
    instanceID_ = instanceID;
    interfaces_implemented_ = {"PluginExecutorInterface", "PluginInterface"};
    Plugins->insertInterfacesImplemented(pluginName_, interfaces_implemented_);
    optionSchema_ = R"(
{
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "type": "object",
    "properties": {
        "plugin": {
            "type": "array",
            "description": "List of plugins to be executed by this executor.",
            "items": {
                "type": "object",
                "properties": {
                    "name": {
                        "type": "string",
                        "description": "The name of the plugin to be executed."
                    },
                    "options": {
                        "type": "object",
                        "description": "Configuration options for this plugin.",
                        "additionalProperties": true
                    }
                },
                "required": [
                    "name",
                    "options"
                ],
                "additionalProperties": false
            }
        },
          "num_threads": {
            "type":"integer",
              "description": "Number of threads to evenly distribute the input EmailList accross."
          }
    },
    "required": [
        "plugin",
      "num_threads"
    ],
    "additionalProperties": true
}
)"_json;
    SET_PLUGIN_STATE("LOADED");
}

bool ParallelPluginExecutor::instantiateRecursive() {
    bool status = true;
    reloadPluginsFromConfig();
    for (auto plugin : managedPlugins_) {
        status &= plugin.second->instantiateRecursive();
    }
    status? SET_PLUGIN_STATE("READY") : SET_PLUGIN_STATE("FAILED");
    return status;
}

bool ParallelPluginExecutor::execute(EmailListView* emailList) {
    LOG_INFO << "ParallelPluginExecutor::execute called.";
    SET_PLUGIN_STATE("RUNNING");
    int numThreads = optionConfig_["num_threads"];
    auto partitions = emailList->split(numThreads);

    std::vector<std::future<bool>> futures;
    for (auto& partition : partitions) {
        futures.push_back(std::async(std::launch::async, [this, &partition]() {
            for (auto& [_, plugin] : managedPlugins_) {
                if (plugin) {
                    bool success = plugin->execute(&partition);
                    if (!success) {
                        SET_PLUGIN_STATE("FAILED");
                        LOG_ERROR << "Plugin execution failed.";
                        return false;
                    }
                }
            }
            partition.commitInserts();
            return true;
        }));
    }

    bool overallSuccess = true;
    for (auto& f : futures) {
        overallSuccess &= f.get();  // Wait for all threads to complete
    }
    overallSuccess ? SET_PLUGIN_STATE("COMPLETE") : SET_PLUGIN_STATE("FAILED");
    return overallSuccess;
}
void ParallelPluginExecutor::clearAllInstances() {
    for (auto a : managedPlugins_) {
        auto* executor = dynamic_cast<PluginExecutorInterface*>(a.second.get());
        if (executor) {
            executor->clearAllInstances();
        }
    }
    managedPlugins_.clear();
}

std::vector<std::string> ParallelPluginExecutor::listPluginsByID() {
    std::vector<std::string> instanceIDs;
    for (const auto& [instanceID, _] : managedPlugins_) {
        instanceIDs.push_back(instanceID);
    }
    return instanceIDs;
}

bool ParallelPluginExecutor::executeOne(EmailListView * emailList, std::string pluginInstanceID) {
    LOG_INFO << "ParallelPluginExecutor::executeOne called. Calling ::execute().";
    return execute(emailList); // parallel and only takes one plugin, so just execute that plugin.
}


std::shared_ptr<PluginInterface> ParallelPluginExecutor::getInstantiatedPluginByID(const std::string& pluginInstanceID) {
    auto it = managedPlugins_.find(pluginInstanceID);
    return (it != managedPlugins_.end()) ? it->second : nullptr;
}

std::string ParallelPluginExecutor::getInstantiatedPluginConfig(const std::string& pluginInstanceID) {
    return managedPlugins_.at(pluginInstanceID)->getConfig();
}

// Update a plugin's configuration
void ParallelPluginExecutor::updateInstantiatedPluginConfig(const std::string& pluginInstanceID, const nlohmann::json& newOptions) {
    if (std::shared_ptr<PluginInterface> plugin = getInstantiatedPluginByID(pluginInstanceID)) {
        plugin->setConfig(newOptions);
        LOG_DEBUG_VERBOSE << "Updated config for plugin: " << pluginInstanceID;
    }
}

// Remove a plugin instance
void ParallelPluginExecutor::removeInstantiatedPlugin(const std::string& pluginInstanceID) {
    managedPlugins_.erase(pluginInstanceID);
}

std::string ParallelPluginExecutor::getInstantiatedPluginStatus(const std::string &pluginInstanceID) {
    if (!managedPlugins_[pluginInstanceID]) return "";
    return managedPlugins_[pluginInstanceID]->getState();
}

bool ParallelPluginExecutor::reloadPluginsFromConfig() {
    if (!validateConfig()) { // Checking my schema is valid before continuing...
        LOG_ERROR << "Schema invalid for " << getPluginName();
        return false;
    }

    for (const auto& pluginData : optionConfig_["plugins"]) {
        std::string pluginName = pluginData["name"];
        nlohmann::json options = pluginData.value("options", nlohmann::json{});

        if (auto pluginInstance = Plugins->createPluginInstance(pluginName, options)) {
            //LOG_DEBUG_VERBOSE << "Loaded plugin: " << pluginName;
            return (managedPlugins_.at(pluginInstance->first) = std::move(pluginInstance->second)).get();
        }
        LOG_ERROR << "Failed to load plugin: " << pluginName;
        SET_PLUGIN_STATE("FAILED");
        return false;
    }
    return true;
}

nlohmann::json ParallelPluginExecutor::printRecursiveInstanceTreeJson() {
    nlohmann::json node;
    try {
        node["instanceID"] = instanceID_;
        node["createFunc"] = Plugins->getCreateFuncForInstance(instanceID_);
        node["state"]     = getState();
        node["schema"]      = optionSchema_;
        node["config"]         = optionConfig_;
        node["children"] = nlohmann::json::array();
        for (auto a : managedPlugins_) {
            node["children"].emplace_back(a.second->printRecursiveInstanceTreeJson());
        }
    } catch (std::exception& e) {
        LOG_ERROR << e.what();
    }
    return node;
}