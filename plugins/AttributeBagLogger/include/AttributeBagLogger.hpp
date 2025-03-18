
#pragma once

#include "PluginRunnableInterface.hpp"

// AttributeBagLogger class implementing PluginInterface
class AttributeBagLogger final : public PluginRunnableInterface {
public:
    explicit AttributeBagLogger(const std::string&);  // Constructor
    ~AttributeBagLogger() override;  // Destructor

    bool instantiateRecursive() override;
    nlohmann::json printRecursiveInstanceTreeJson() override;

    bool execute(EmailListView * emailList) override;
private:
    struct Register {
        Register() {
            std::string name = "AttributeBagLogger";
            LOG_DEBUG_VERBOSE << "Registering plugin " << name;
            Plugins->registerPlugin(name, [](const std::string& instanceID) -> PluginInterface* {
                return new AttributeBagLogger(instanceID);
            });
        }
    };
    static inline Register reg;
};
