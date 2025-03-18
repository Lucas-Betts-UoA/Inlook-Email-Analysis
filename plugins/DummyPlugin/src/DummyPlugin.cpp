#include "DummyPlugin.hpp"

#include "Logger.hpp"
#include "EmailListView.hpp"
#include <PluginInterface.hpp>
#include <unordered_map>
#include <iostream>
#include <functional>
#include <string>
#include <regex>
#include "Email.hpp"
#include <vector>
#include <filesystem>

// Constructor
DummyPlugin::DummyPlugin(const std::string& instanceID) : PluginRunnableInterface(instanceID) {
    pluginName_ = "DummyPlugin";
    instanceID_ = instanceID;
    optionSchema_ = R"(
{
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "type": "object",
    "properties": {
    },
    "required": [
    ],
    "additionalProperties": false
}
    )"_json;
    inputAttributes_ = {};
    generatedAttributes_ = {};
    SET_PLUGIN_STATE("LOADED");
}

bool DummyPlugin::instantiateRecursive() {
    SET_PLUGIN_STATE("READY");
    return true;
}

nlohmann::json DummyPlugin::printRecursiveInstanceTreeJson() {
    nlohmann::json node;
    try {
        node["instanceID"] = instanceID_;
        node["createFunc"] = Plugins->getCreateFuncForInstance(instanceID_) ? Plugins->getCreateFuncForInstance(instanceID_) : "Not Loaded";
        node["state"]     = getState();
        node["schema"]       = !optionSchema_.empty()? optionSchema_ : "";
        node["config"]       = !optionConfig_.empty() ? optionConfig_ : "";
    } catch (std::exception& e) {
        LOG_ERROR << e.what();
    }
    return node;
}

// Destructor
DummyPlugin::~DummyPlugin() = default;

bool DummyPlugin::execute(EmailListView * emailList) {
    LOG_INFO << "DummyPlugin::execute called.";
    SET_PLUGIN_STATE("RUNNING");
    for (auto email : *emailList) {
        LOG_DEBUG_VERBOSE << "Email " << email.getUniqueHash() << " Parsed by DummyPlugin";
    }
    SET_PLUGIN_STATE("COMPLETE");
    return true;
}