#include "PythonScriptExecutor.hpp"

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
#include <pybind11/embed.h>
#include  <pybind11/stl.h>

namespace py = pybind11;

// Constructor
PythonScriptExecutor::PythonScriptExecutor(const std::string& instanceID) : PluginRunnableInterface(instanceID) {
    pluginName_ = "PythonScriptExecutor";
    instanceID_ = instanceID;
    optionSchema_ = R"(
    {
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "type": "object",
    "properties": {
      "scriptPath": {
        "type": "string",
        "description": "The path to the Python scripts."
      }
    },
    "required": [
      "scriptPath"
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

bool PythonScriptExecutor::execute(EmailListView *emailList) {
    LOG_INFO << "PythonScriptExecutor::execute called.";
    SET_PLUGIN_STATE("RUNNING");
    try {
        LOG_INFO << "Acquiring GIL";
        py::gil_scoped_acquire gil;
        LOG_INFO << "GIL acquired";

        std::filesystem::path cwd = optionConfig_["scriptPath"];

        if (!std::filesystem::exists(cwd)) {
            std::string errorMsg = "Script directory does not exist: " + cwd.string();
            LOG_ERROR << errorMsg;
            throw std::runtime_error(errorMsg);
        }

        py::module sys = py::module::import("sys");
        sys.attr("path").attr("append")(cwd.string());

        nlohmann::json emailArray = nlohmann::json::array();
        for (auto& email : *emailList) {
            emailArray.push_back(email.toJson());
        }
        py::module script = py::module::import("email_plugin_test");
        py::object processFunc = script.attr("process_emails");

        py::object result = processFunc(emailArray.dump());

        nlohmann::json processedEmails = nlohmann::json::parse(result.cast<std::string>());

        for (const auto& emailJson : processedEmails) {
            Email email;
            email.fromJson(emailJson);
            emailList->insertEmail(email);
        }

        emailList->commitInserts();
    }
    catch (const std::exception& e) {
        LOG_ERROR << "PythonScriptExecutor exception: " << e.what();
        SET_PLUGIN_STATE("FAILED");
        return false;
    }

    SET_PLUGIN_STATE("COMPLETE");
    return true;
}
