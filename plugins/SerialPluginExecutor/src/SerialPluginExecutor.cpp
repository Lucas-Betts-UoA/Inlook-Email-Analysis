#include "SerialPluginExecutor.hpp"
#include "Logger.hpp"
#include "EmailListView.hpp"
#include "PluginInterface.hpp"
#include "PluginRegistry.hpp"

SerialPluginExecutor::SerialPluginExecutor(const std::string& instanceID) : PluginExecutorInterface(instanceID) {
    pluginName_ = "RootPluginExecutor";
    instanceID_ = instanceID;
    interfaces_implemented_ = {"PluginExecutorInterface", "PluginInterface"};
    Plugins->insertInterfacesImplemented(pluginName_, interfaces_implemented_);
    optionSchema_ = R"(
{
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "type": "object",
    "properties": {
        "plugins": {
            "type": "array",
            "description": "List of plugins to be executed by this executor.",
            "items": {
                "type": "object",
                "properties": {
                    "name": {
                        "type": "string",
                        "description": "The name of the plugin to be executed.",
                        "_inlook_check": {
                            "_PluginRegistry": "PluginExecutorInterface"
                        }
                    },
                    "options": {
                        "type": "object",
                        "description": "Configuration options for this plugin.",
                        "additionalProperties": true
                    }
                },
                "required": [
                    "name"
                ],
                "additionalProperties": false
            }
        }
    },
    "required": [
        "plugins"
    ],
    "additionalProperties": true
}
)"_json;
    SET_PLUGIN_STATE("LOADED");

}


bool SerialPluginExecutor::instantiateRecursive() {
    bool status = true;
    reloadPluginsFromConfig();
    for (auto plugin : managedPlugins_) {
        status &= plugin.second->instantiateRecursive();
    }
    status? SET_PLUGIN_STATE("READY") : SET_PLUGIN_STATE("FAILED");
    return status;
}

bool SerialPluginExecutor::execute(EmailListView * emailList) {
    LOG_INFO << "SerialPluginExecutor::execute called.";

    if (!emailList || managedPlugins_.empty()) {
        LOG_ERROR << "EmailList is null or no plugins are registered.";
        return false;
    }

    bool status = true;
    SET_PLUGIN_STATE("RUNNING");
    for (auto a : managedPlugins_) {
        if (a.second) {
            status &= a.second->execute(emailList);
            emailList->commitInserts();
        } else {
            LOG_ERROR << "SPE Plugin being asked for doesn't exist!";
            status &= false;
            return status;
        }
    }
    SET_PLUGIN_STATE("COMPLETE");
    return status;
}

void SerialPluginExecutor::clearAllInstances() {
    for (auto a : managedPlugins_) {
        auto* executor = dynamic_cast<PluginExecutorInterface*>(a.second.get());
        if (executor) {
            executor->clearAllInstances();
        }
    }
    managedPlugins_.clear();
}

nlohmann::json SerialPluginExecutor::printRecursiveInstanceTreeJson() {
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

// Retrieve a plugin by instance ID
std::shared_ptr<PluginInterface> SerialPluginExecutor::getInstantiatedPluginByID(const std::string& pluginInstanceID) {
    return managedPlugins_[pluginInstanceID];
}

// Update a plugin's configuration
void SerialPluginExecutor::updateInstantiatedPluginConfig(const std::string& pluginInstanceID, const nlohmann::json& newOptions) {
    if (std::shared_ptr<PluginInterface> plugin = getInstantiatedPluginByID(pluginInstanceID)) {
        plugin->setConfig(newOptions);
        LOG_DEBUG_VERBOSE << "Updated config for plugin: " << pluginInstanceID;
    }
}

// Remove a plugin instance
void SerialPluginExecutor::removeInstantiatedPlugin(const std::string& pluginInstanceID) {
    managedPlugins_.erase(pluginInstanceID);
}

bool SerialPluginExecutor::reloadPluginsFromConfig() {
    if (!validateConfig()) { // Checking my schema is valid before continuing...
        LOG_ERROR << "Schema invalid for " << getPluginName();
        return false;
    }

    for (const auto& pluginData : optionConfig_["plugins"]) {
        std::string pluginName = pluginData["name"];
        nlohmann::json options = pluginData.value("options", nlohmann::json{});

        if (auto pluginInstance = Plugins->createPluginInstance(pluginName, options)) {
            //LOG_DEBUG_VERBOSE << "Loaded plugin: " << pluginName;
            managedPlugins_[pluginInstance->first] = std::move(pluginInstance->second);
        } else {
            LOG_ERROR << "Failed to load plugin: " << pluginName;
            SET_PLUGIN_STATE("FAILED");
            return false;
        }
    }

    return true;
}

std::string SerialPluginExecutor::getInstantiatedPluginConfig(const std::string& pluginInstanceID) {
    return managedPlugins_[pluginInstanceID]->getConfig();
}

// Get the status of a plugin
std::string SerialPluginExecutor::getInstantiatedPluginStatus(const std::string &pluginInstanceID) {
    if (!managedPlugins_[pluginInstanceID]) return "";
    return managedPlugins_[pluginInstanceID]->getState();
}

// List all managed plugin instance IDs
std::vector<std::string> SerialPluginExecutor::listPluginsByID() {
    std::vector<std::string> instanceIDs;
    for (const auto& [instanceID, _] : managedPlugins_) {
        instanceIDs.push_back(instanceID);
    }
    return instanceIDs;
}

bool SerialPluginExecutor::executeOne(EmailListView * emailList, std::string instanceID) {
    LOG_INFO << "SerialPluginExecutor::executeOne called.";

    if (!emailList || managedPlugins_.empty()) {
        LOG_ERROR << "EmailList is null or no plugins are registered.";
        return false;
    }

    bool status = true;
    SET_PLUGIN_STATE("RUNNING");
    for (auto a : managedPlugins_) {
        if (!a.second) {
            LOG_ERROR << "SPE Plugin being asked for doesn't exist!";
            SET_PLUGIN_STATE("FAILED");
            status &= false;
            return status;
        }
        if (a.first == instanceID) {
            LOG_DEBUG_VERBOSE << "SPE Executing plugin: " << a.first << " as any prior plugins are COMPLETE.";
            status &= a.second->execute(emailList);
            emailList->commitInserts();
            return status;
        }
        if (a.second->getState() != "COMPLETE") {
            LOG_DEBUG_VERBOSE << "SPE prior plugin:" << a.first << "is not complete before asking to execute: " << instanceID << ".";
            SET_PLUGIN_STATE("READY");
            break;
        }
    }
    SET_PLUGIN_STATE("READY");
    return status;
}


