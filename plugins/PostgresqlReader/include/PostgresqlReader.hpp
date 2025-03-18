#pragma once
#include "PluginRunnableInterface.hpp"
#include <pqxx/pqxx>

// PostgresqlReader class implementing PluginInterface
class PostgresqlReader final : public PluginRunnableInterface {
public:
    explicit PostgresqlReader(const std::string&);  // Constructor
    ~PostgresqlReader() override;  // Destructor
    bool instantiateRecursive() override;
    nlohmann::json printRecursiveInstanceTreeJson() override;

    bool execute(EmailListView * emailList) override;
private:
    void getEmails(pqxx::result &result, pqxx::work& trans, EmailListView *emailList);
    /* Self registration for plugin registry
     struct Register {
         Register() {
             std::string name = "PostgresqlReader";
             static auto pluginSchema = R"(
  {
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "type": "object",
    "properties": {
      "databasePath": {
        "type": "string",
        "description": "The path to the PostgreSQL database."
      },
      "schemaName": {
        "type": "string",
        "description": "The name of the schema."
      },
      "datasetName": {
        "type": "string",
        "description": "The name of the dataset."
      },
      "filters": {
        "type": "array",
        "items": {
          "type": "object",
          "properties": {
            "columnName": {
              "type": "string",
              "description": "The name of the column to filter."
            },
            "condition": {
              "type": "string",
              "description": "The condition operator."
            },
            "value": {
              "type": "string",
              "description": "The value to filter by."
            }
          },
          "required": [
            "columnName",
            "condition",
            "value"
          ],
          "additionalProperties": false
        },
        "description": "A list of filters to apply."
      }
    },
    "required": [
      "databasePath",
      "schemaName",
      "datasetName",
      "filters"
    ],
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
            std::string name = "PostgresqlReader";
            LOG_DEBUG_VERBOSE << "Registering plugin " << name;
            Plugins->registerPlugin(name, [](const std::string& instanceID) -> PluginInterface* {
                return new PostgresqlReader(instanceID);
            });
        }
    };
    static inline Register reg;
};