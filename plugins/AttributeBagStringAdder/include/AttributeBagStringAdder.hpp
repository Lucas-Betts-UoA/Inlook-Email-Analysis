
#pragma once
#include "PluginRunnableInterface.hpp"

// AttributeBagStringAdder class implementing PluginInterface
class AttributeBagStringAdder final : public PluginRunnableInterface {
public:
    AttributeBagStringAdder(const std::string&);  // Constructor
    ~AttributeBagStringAdder() override;  // Destructor

    bool instantiateRecursive() override;
    nlohmann::json printRecursiveInstanceTreeJson() override;

    bool execute(EmailListView * emailList) override;

private:
    struct Register {
        Register() {
            std::string name = "AttributeBagStringAdder";
            LOG_DEBUG_VERBOSE << "Registering plugin " << name;
            Plugins->registerPlugin(name, [](const std::string& instanceID) -> PluginInterface* {
                return new AttributeBagStringAdder(instanceID);
            });
        }
    };
    static inline Register reg;
};
