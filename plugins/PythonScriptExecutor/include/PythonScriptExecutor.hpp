
#pragma once
#include "PluginRunnableInterface.hpp"


// PythonScriptExecutor class implementing PluginInterface
class  final : public PluginRunnableInterface {
public:
    explicit PythonScriptExecutor(const std::string&);  // Constructor
    ~PythonScriptExecutor() override;  // Destructor

    bool instantiateRecursive() override;
    nlohmann::json printRecursiveInstanceTreeJson() override;

    int execute(EmailList *emailList) override;

private:
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
