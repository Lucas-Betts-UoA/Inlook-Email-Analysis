#pragma once
#include "PluginRunnableInterface.hpp"

// DummyPlugin class implementing PluginInterface
class DummyPlugin final : public PluginRunnableInterface {
public:
    explicit DummyPlugin(const std::string&);  // Constructor
    ~DummyPlugin() override;  // Destructor

    bool instantiateRecursive() override;
    nlohmann::json printRecursiveInstanceTreeJson() override;

    bool execute(EmailListView * emailList) override;

private:
    struct Register {
        Register() {
            std::string name = "DummyPlugin";
            LOG_DEBUG_VERBOSE << "Registering plugin " << name;
            Plugins->registerPlugin(name, [](const std::string& instanceID) -> PluginInterface* {
                return new DummyPlugin(instanceID);
            });
        }
    };
    static inline Register reg;
};