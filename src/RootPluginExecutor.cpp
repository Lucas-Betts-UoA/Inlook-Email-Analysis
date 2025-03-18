#include <unistd.h>

#include "EmailListView.hpp"
#include "RootPluginExecutor.hpp"
#include "PluginExecutorInterface.hpp"
#include "PluginRegistry.hpp"

// RootPluginExecutor acts as the root of the execution tree, and executes only a single plugin.
// Typically, this would be an executor (such as SerialPluginExecutor), however it can be a plugin if needed.

RootPluginExecutor::RootPluginExecutor(const std::string& instanceID) : PluginExecutorInterface(instanceID){
    pluginName_ = "RootPluginExecutor";
    instanceID_ = instanceID;
    interfaces_implemented_ = {"PluginExecutorInterface", "PluginInterface"};
    Plugins->insertInterfacesImplemented(pluginName_, interfaces_implemented_);
    optionSchema_ = R"(
{
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "type": "object",
    "properties": {
        "name": {
            "type": "string",
            "description": "The name of the outer plugin executor (i.e. SerialPluginExecutor), to be given the outermost options object.",
            "_inlook_check": {
                "_PluginRegistry": "PluginExecutorInterface"
            }
        },
        "options": {
            "type": "object",
            "description": "The configuration options provided to the outermost executor object."
        },
        "additionalProperties": false
    },
    "required": ["name", "options"],
    "additionalProperties": false
}
            )"_json;
    SET_PLUGIN_STATE("UNLOADED");
}

bool RootPluginExecutor::instantiateRecursive() {
    bool status = true;
    status &= reloadPluginsFromConfig();
    if (!status) {
        SET_PLUGIN_STATE("FAILED");
        return status;
    }
    status &= primaryPlugin_->instantiateRecursive();
    status? SET_PLUGIN_STATE("READY") : SET_PLUGIN_STATE("FAILED");
    return status;
}

bool RootPluginExecutor::execute(EmailListView * emailList) {
    LOG_INFO << "RootPluginExecutor::execute called.";
    SET_PLUGIN_STATE("RUNNING");
    bool status = false;
    if (primaryPlugin_) {
        status = primaryPlugin_->execute(emailList);
        emailList->commitInserts();
    }
    status ? SET_PLUGIN_STATE("COMPLETE") : SET_PLUGIN_STATE("FAILED");
    return status;
}

bool RootPluginExecutor::executeOne(EmailListView * emailList, std::string instanceID) {
    LOG_INFO << "RootPluginExecutor::executeOne called.";
    SET_PLUGIN_STATE("RUNNING");
    bool status = false;
    if (primaryPluginInstanceID_ == instanceID) {
        status = primaryPlugin_->execute(emailList);
        emailList->commitInserts();
    }
    status ? SET_PLUGIN_STATE("COMPLETE") : SET_PLUGIN_STATE("FAILED");
    return status;
}

void RootPluginExecutor::clearAllInstances() {
    SET_PLUGIN_STATE("FAILED");
    SET_PLUGIN_STATE("UNLOADED"); // Follow available states back to loaded.
    if (primaryPlugin_) {
        auto* executor = dynamic_cast<PluginExecutorInterface*>(primaryPlugin_.get());
        if (executor) {
            executor->clearAllInstances();
        }
        primaryPlugin_.reset();
    }
    SET_PLUGIN_STATE("LOADED"); // Follow available states back to loaded.
}

std::vector<std::string> RootPluginExecutor::listPluginsByID() {
    if (primaryPlugin_) {
        return { primaryPluginInstanceID_ };
    }
    return {};
}

std::shared_ptr<PluginInterface> RootPluginExecutor::getInstantiatedPluginByID(const std::string &_) {
    if (primaryPlugin_) {
        return { primaryPlugin_ };
    }
    return {};
}

std::string RootPluginExecutor::getInstantiatedPluginConfig(const std::string&_) {
    return primaryPlugin_->getConfig();
}

void RootPluginExecutor::updateInstantiatedPluginConfig(const std::string& _, const nlohmann::json& options) {
    primaryPlugin_->setConfig(options);
}

void RootPluginExecutor::removeInstantiatedPlugin(const std::string& /*pluginInstanceID*/) {
    clearAllInstances();
}

std::string RootPluginExecutor::getInstantiatedPluginStatus(const std::string & /*pluginInstanceID*/) {
    return primaryPlugin_->getState();
}

bool RootPluginExecutor::reloadPluginsFromConfig() {
    if (!validateConfig()) {
        LOG_ERROR << "Schema invalid for " << getPluginName();
        return false;
    }
    SET_PLUGIN_STATE("LOADED");

    std::string pluginName = optionConfig_["name"].get<std::string>();
    nlohmann::json pluginExecutorConfig = optionConfig_["options"];
    auto pluginInstance = Plugins->createPluginInstance(pluginName, pluginExecutorConfig);
    if (!pluginInstance) {
        LOG_ERROR << "Failed to load plugin: " << pluginName << ". Keeping previous plugin alive.";
        SET_PLUGIN_STATE("FAILED");
        return false;
    }

    primaryPluginInstanceID_ = pluginInstance->first;
    primaryPlugin_ = std::move(pluginInstance->second);
    return true;
}

nlohmann::json RootPluginExecutor::printRecursiveInstanceTreeJson() {
    nlohmann::json node;
    try {
        node["instanceID"] = instanceID_;
        node["createFunc"] = Plugins->getCreateFuncForInstance(instanceID_) ? Plugins->getCreateFuncForInstance(instanceID_) : "Not Loaded";
        node["state"]     = getState();
        node["schema"]       = !optionSchema_.empty()? optionSchema_ : R"({})"_json;
        node["config"]       = !optionConfig_.empty() ? optionConfig_ : R"({})"_json;
        node["children"] = nlohmann::json::array();
        if (primaryPlugin_) node["children"].emplace_back(primaryPlugin_->printRecursiveInstanceTreeJson());
    } catch (std::exception& e) {
        LOG_ERROR << e.what();
    }
    return node;
}