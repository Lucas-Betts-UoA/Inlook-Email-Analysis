#pragma once

#include "PluginRunnableInterface.hpp"
#include <pqxx/pqxx>

class Email;
// PostgresqlSaver class implementing PluginInterface
class PostgresqlSaver final : public PluginRunnableInterface {
public:
    explicit PostgresqlSaver(const std::string&);  // Constructor

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
    struct Register {
      Register() {
        std::string name = "PostgresqlSaver";
        LOG_DEBUG_VERBOSE << "Registering plugin " << name;
        Plugins->registerPlugin(name, [](const std::string& instanceID) -> PluginInterface* {
          return new PostgresqlSaver(instanceID);
        });
      }
    };
    static inline Register reg;
};