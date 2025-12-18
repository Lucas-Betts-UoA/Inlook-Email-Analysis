
#include "PythonScriptExecutor.hpp"

#include "Logger.hpp"
#include "EmailList.hpp"
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
PythonScriptExecutor::PythonScriptExecutor(const std::string& instanceID) : PluginRunnableInterface(instanceID) {
    pluginName_ = "PythonScriptExecutor";
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

bool PythonScriptExecutor::instantiateRecursive() {
    SET_PLUGIN_STATE("READY");
    return true;
}

nlohmann::json PythonScriptExecutor::printRecursiveInstanceTreeJson() {
nlohmann::json node;
node["instanceID"] = instanceID_;
node["createFunc"] = Plugins->getCreateFuncForInstance(instanceID_);
node["state"]     = getState();
return node;
}

// Destructor
PythonScriptExecutor::~PythonScriptExecutor() = default;

int PythonScriptExecutor::execute(EmailList *emailList) {
LOG_INFO << "PythonScriptExecutor::execute called.";
SET_PLUGIN_STATE("RUNNING");
for (auto email : *emailList) {
    LOG_DEBUG_VERBOSE << "Email Parsed by PythonScriptExecutor";
}
SET_PLUGIN_STATE("COMPLETE");
return true;
}
