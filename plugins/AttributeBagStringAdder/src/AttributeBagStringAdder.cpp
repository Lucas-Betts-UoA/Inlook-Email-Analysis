#include "AttributeBagStringAdder.hpp"
#include "AttributeBagValueInterface.hpp"
#include "PluginInterface.hpp"
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
#include "EmailListView.hpp"

// Exported function to create a new instance of FirstPlugin
AttributeBagStringAdder::AttributeBagStringAdder(const std::string& instanceID) : PluginRunnableInterface(instanceID) {
    pluginName_ = "AttributeBagStringAdder";
    instanceID_ = instanceID;
    optionSchema_ = R"(
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "object",
  "properties": {
    "attributes": {
      "type": "array",
      "items": {
        "type": "object",
        "properties": {
          "attributeKey": {
            "type": "string",
            "description": "The key for the attribute to be added."
          },
          "attributeVal": {
            "type": "string",
            "description": "The value for the attribute to be added."
          }
        },
        "required": [
          "attributeKey",
          "attributeVal"
        ],
        "additionalProperties": false
      },
      "description": "A list of attributes to add to the attribute bag."
    }
  },
  "required": [
    "attributes"
  ],
  "additionalProperties": false
}
    )"_json;
    inputAttributes_ = {};
    generatedAttributes_ = {};
    SET_PLUGIN_STATE("LOADED");
}

// Destructor
AttributeBagStringAdder::~AttributeBagStringAdder() = default;

bool AttributeBagStringAdder::instantiateRecursive() {
    SET_PLUGIN_STATE("READY");
    return true;
}

nlohmann::json AttributeBagStringAdder::printRecursiveInstanceTreeJson() {
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


bool AttributeBagStringAdder::execute(EmailListView * emailList) {
    SET_PLUGIN_STATE("RUNNING");
    for (Email& email : *emailList) {
        for (const auto& attribute : optionConfig_["attributes"]) {
            email.insertAttribute(attribute["attributeKey"].get<std::string>(), std::make_unique<AttributeBagString>(AttributeBagString(attribute["attributeVal"].get<std::string>())));
        }
    }
    SET_PLUGIN_STATE("COMPLETE");
    return true;
}