
#include "AttributeBagLogger.hpp"

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
AttributeBagLogger::AttributeBagLogger(const std::string& instanceID) : PluginRunnableInterface(instanceID) {
    pluginName_ = "AttributeBagLogger";
    instanceID_ = instanceID;
    optionSchema_ = R"(
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "object",
  "properties": {
    "logByFile": {
      "type": "boolean",
      "description": "Whether to filter logging by specific files"
    },
    "files": {
      "type": "array",
      "items": {
        "type": "object",
        "properties": {
          "file": {
            "type": "string",
            "description": "File identifier to match against email attributes"
          }
        },
        "required": ["file"],
        "additionalProperties": false
      },
      "description": "List of files to log, if logByFile is true"
    }
  },
    "required": ["logByFile"],
    "if": {
        "properties": {
          "logByFile": { "const": true }
        }
    },
    "then": {
        "required": ["files"]
    },
    "additionalProperties": false,
    "description": "Logs email attributes, optionally filtered by specific files"
    }
    )"_json;
    inputAttributes_ = {};
    generatedAttributes_ = {};
    SET_PLUGIN_STATE("LOADED");
}

bool AttributeBagLogger::instantiateRecursive() {
    SET_PLUGIN_STATE("READY");
    return true;
}

nlohmann::json AttributeBagLogger::printRecursiveInstanceTreeJson() {
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
AttributeBagLogger::~AttributeBagLogger() = default;

bool AttributeBagLogger::execute(EmailListView * emailList) {
    LOG_INFO << "AttributeBagLogger::execute called.";
    SET_PLUGIN_STATE("RUNNING");
    if (optionConfig_["logByFile"].get<bool>()) {
        auto& numOfFiles = optionConfig_["files"];
        LOG_INFO << "Reading " << std::to_string(numOfFiles.size()) << " emails' attributes.";
    } else {
        LOG_INFO << "Reading " << emailList->getSize() << " emails' attributes";
    }
    for (const auto& email : *emailList) {
        if (optionConfig_["logByFile"].get<bool>()) {
            for (const auto& files : optionConfig_["files"]) {
                if (files["file"].get<std::string>() == email.getAttributeValue("File identifier")->toString()) {
                    for (const auto& attKeys : email.getAttributeKeys()) {
                        try {
                            std::string attVal = email.getAttributeValue(attKeys)->toString();
                            LOG_INFO << attKeys << ": " << attVal;
                        } catch (const std::exception& e) {
                            LOG_ERROR << "Exception thrown: " << e.what();
                            LOG_INFO << "Attribute type: " << typeid(email.getAttributeValue(attKeys)).name();
                        }

                    }
                }
            }
        } else {
            for (const auto& attKeys : email.getAttributeKeys()) {
                try {
                    if (attKeys != "File bytes") {
                        std::string attVal = email.getAttributeValue(attKeys)->toString();
                        LOG_INFO << attKeys << ": " << attVal;
                    }
                } catch (const std::exception& e) {
                    LOG_ERROR << "Exception thrown: " << e.what();
                    LOG_INFO << "Attribute type: " << typeid(email.getAttributeValue(attKeys)).name();
                }

            }
        }
    }
    SET_PLUGIN_STATE("COMPLETE");
    return true;
}