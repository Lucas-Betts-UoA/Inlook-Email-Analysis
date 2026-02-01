#pragma once
#include "PluginRunnableInterface.hpp"
#include <pybind11/embed.h>
#include <nlohmann/json.hpp>
#include <pybind11/embed.h>

namespace py = pybind11;
// PythonScriptExecutor class implementing PluginInterface
class PythonScriptExecutor final : public PluginRunnableInterface {
public:
    explicit PythonScriptExecutor(const std::string&);  // Constructor
    ~PythonScriptExecutor() override;  // Destructor

    bool instantiateRecursive() override;
    nlohmann::json printRecursiveInstanceTreeJson() override;

    bool execute(EmailListView *emailList) override;

private:
    py::scoped_interpreter python{};
    py::gil_scoped_release release;
    struct Register {
        Register() {
            std::string name = "PythonScriptExecutor";
            LOG_DEBUG_VERBOSE << "Registering plugin " << name;
            Plugins->registerPlugin(name, [](const std::string& instanceID) -> PluginInterface* {
                return new PythonScriptExecutor(instanceID);
            });
        }
    };
    static inline Register reg;
};
