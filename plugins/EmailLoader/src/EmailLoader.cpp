#include "Logger.hpp"
#include "EmailListView.hpp"
#include <unordered_map>
#include <iostream>
#include <functional>
#include <string>
#include <regex>
#include "Email.hpp"
#include <vector>
#include "EmailParser_FSM.hpp"
#include <filesystem>
#include "EmailListView.hpp"
#include "EmailLoader.hpp"
#include "PluginInterface.hpp"

// Constructor
EmailLoader::EmailLoader(const std::string& instanceID) :
    ::PluginRunnableInterface(instanceID) {
    pluginName_ = "EmailLoader";
    instanceID_ = instanceID;
    optionSchema_ = R"(
{
      "$schema": "https://json-schema.org/draft/2020-12/schema",
      "type": "object",
      "properties": {
        "emailPath": {
          "type": "string",
          "description": "Path to an email file or a directory. If a directory, it will be traversed recursively."
        }
      },
      "required": ["emailPath"],
      "additionalProperties": false
    }
    )"_json;
    inputAttributes_ = {};
    generatedAttributes_ = {"headerList", "mimebodyList", "bodyList"};
    SET_PLUGIN_STATE("LOADED");
}

// Destructor
EmailLoader::~EmailLoader() = default;

bool EmailLoader::instantiateRecursive() {
    SET_PLUGIN_STATE("READY");
    return true;
}

nlohmann::json EmailLoader::printRecursiveInstanceTreeJson() {
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

bool EmailLoader::execute(EmailListView * emailList) {
    LOG_INFO << "EmailLoader::execute called.";
    SET_PLUGIN_STATE("RUNNING");
    try {
        EmailParser_FSM fsm(emailList);
        std::filesystem::path p = optionConfig_["emailPath"];
        fsm.parse(p);
    } catch (std::exception& e) {
        SET_PLUGIN_STATE("FAILED");
        LOG_ERROR << e.what();
        return false;
    }
    SET_PLUGIN_STATE("COMPLETE");
    return true;
}