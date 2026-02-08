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
#include "EmailBindings.hpp"

namespace py = pybind11;
namespace fs = std::filesystem;

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

void PythonScriptExecutor::ensure_python() {
    static std::once_flag once;
    std::call_once(once, [] {
        static auto *interp = new py::scoped_interpreter(); // Allocate so it isn't unloaded after the first plugin.
        (void)interp;
    });
}

bool PythonScriptExecutor::execute(EmailListView *emailList) {
    LOG_INFO << "PythonScriptExecutor::execute called.";
    SET_PLUGIN_STATE("RUNNING");
    try {
        ensure_python(); // Should create just a single interpreter rather than in the constructor called per plugin load.
        py::gil_scoped_acquire gil;

        fs::path scriptPath = optionConfig_["scriptPath"];

        if (!std::filesystem::exists(scriptPath)) {
            std::string errorMsg = "Script directory does not exist: " + scriptPath.string();
            LOG_ERROR << errorMsg;
            throw std::runtime_error(errorMsg);
        }

        py::module::import("email_core");

        py::module sys = py::module::import("sys");
        sys.attr("path").attr("append")(scriptPath.string());


        py::list pyEmailList;
        for (auto& email : *emailList) {
            pyEmailList.append(py::cast(&email, py::return_value_policy::reference));
        }

        for (const auto& fileEntry : fs::recursive_directory_iterator(scriptPath)) {
            if (fileEntry.is_regular_file() && fileEntry.path().extension() == ".py") {
                if (fileEntry.path().filename() == "__init__.py") continue;


                fs::path relativePath;

                try {
                    std::string parentDir = fileEntry.path().parent_path().string();

                    sys.attr("path").attr("insert")(0, parentDir);

                    std::string moduleName = fileEntry.path().stem().string();


                    LOG_INFO << "Executing loose script: " << moduleName << " from " << parentDir;

                    py::module script = py::module::import(moduleName.c_str());

                    if (py::hasattr(script, "process_emails")) {
                        LOG_INFO << "Executing 'process_emails' in: " << moduleName;
                        py::object processFunc = script.attr("process_emails");
                        processFunc(pyEmailList);
                    } else {
                        LOG_WARNING << "Skipping " << moduleName << ": 'process_emails' function not found.";
                    }
                    sys.attr("path").attr("remove")(parentDir);
                } catch (const std::exception& e) {
                    LOG_ERROR << "Error executing script " << fileEntry.path().string() << ": " << e.what();
                }
            }
        }
    }
    catch (const std::exception& e) {
        LOG_ERROR << "PythonScriptExecutor exception: " << e.what();
        SET_PLUGIN_STATE("FAILED");
        return false;
    }
    SET_PLUGIN_STATE("COMPLETE");
    return true;
}
