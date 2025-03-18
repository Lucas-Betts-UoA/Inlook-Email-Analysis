#pragma once

#include "PluginRunnableInterface.hpp"
#include <pqxx/pqxx>

class Email;
// PostgresqlSaver class implementing PluginInterface
class PostgresqlSaver final : public PluginRunnableInterface {
public:
    explicit PostgresqlSaver(std::string&);  // Constructor

    bool instantiateRecursive() override;
    nlohmann::json printRecursiveInstanceTreeJson() override;

    ~PostgresqlSaver() override;  // Destructor
    bool execute(EmailListView *) override;

private:
    int getOrCreateDataset(pqxx::work& trans);
    void clearDatabase();
    int addEmail(pqxx::work& trans, int datasetid, const Email& email);
    int addHeaderKey(pqxx::work& trans, int emailid, const std::string& key);
    void addHeaderValue(pqxx::work& trans, int headerkeyid, const std::string& value);
    int addEmailPart(pqxx::work& trans, int emailid, const std::string& partBody);
    int addEmailPartHeaderKey(pqxx::work& trans, int emailpartid, const std::string& key);
    void addEmailPartHeaderValue(pqxx::work& trans, int emailpartheaderkeyid, const std::string& value);
    void addAttribute(pqxx::work& trans, int emailid,const std::string& attributekey, const std::string& attributeval);
    /* Self registration for plugin registry
     struct Register {
         Register() {
             std::string name = "PostgresqlSaver";
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
      "datasetDescription": {
        "type": "string",
        "description": "A description of the dataset."
      }
    },
    "required": [
      "databasePath",
      "schemaName",
      "datasetName",
      "datasetDescription"
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

};