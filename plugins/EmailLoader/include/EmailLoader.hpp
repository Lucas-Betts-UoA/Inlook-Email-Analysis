#pragma once

#include "PluginRunnableInterface.hpp"

// EmailLoader class implementing PluginInterface
class EmailLoader final : public PluginRunnableInterface {
public:
    EmailLoader(const std::string&);  // Constructor
    ~EmailLoader() override;  // Destructor

    bool instantiateRecursive() override;
    nlohmann::json printRecursiveInstanceTreeJson() override;

    bool execute(EmailListView * emailList) override;
private:
    /* Self registration for plugin registry
       struct Register {
           Register() {
               std::string name = "EmailLoader";
               static auto pluginSchema = R"(
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
               LOG_DEBUG_VERBOSE << "Registering plugin " << name;
               Plugins->registerStaticPlugin(name, pluginSchema, []() -> PluginInterface* {
                   return new SerialPluginExecutor();
               });
           }
       };
       static inline Register reg;
       */
    struct Register {
        Register() {
            std::string name = "EmailLoader";
            LOG_DEBUG_VERBOSE << "Registering plugin " << name;
            Plugins->registerPlugin(name, [](const std::string& instanceID) -> PluginInterface* {
                return new EmailLoader(instanceID);
            });
        }
    };
    static inline Register reg;
};