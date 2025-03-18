#include "PostgresqlSaver.hpp"
#include "PluginInterface.hpp"
#include "Logger.hpp"
#include "EmailListView.hpp"
#include <unordered_map>
#include <iostream>
#include <functional>
#include <string>
#include <regex>
#include "Email.hpp"
#include <vector>
#include <filesystem>
#include <pqxx/pqxx>
#include <ctime>

// Constructor
PostgresqlSaver::PostgresqlSaver(std::string& instanceID) : PluginRunnableInterface(instanceID) {
    pluginName_ = "DummyPlugin";
    instanceID_ = instanceID;
    optionSchema_ = R"(
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
    inputAttributes_ = {};
    generatedAttributes_ = {};
    SET_PLUGIN_STATE("LOADED");
}

bool PostgresqlSaver::instantiateRecursive() {
    SET_PLUGIN_STATE("READY");
    return true;
}

nlohmann::json PostgresqlSaver::printRecursiveInstanceTreeJson() {
    nlohmann::json node;
    try {
        node["instanceID"] = instanceID_;
        node["createFunc"] = Plugins->getCreateFuncForInstance(instanceID_) ? Plugins->getCreateFuncForInstance(instanceID_) : "Not Loaded";
        node["state"]     = getState();
        node["schema"]       = !optionSchema_.empty()? optionSchema_ : "";
        node["config"]       = !optionConfig_.empty() ? optionConfig_ : "";
    } catch (std::exception& e) {
        LOG_ERROR << e.what();
    }
    return node;
}

// Destructor
PostgresqlSaver::~PostgresqlSaver() = default;



void PostgresqlSaver::clearDatabase() {
    pqxx::connection cx{optionConfig_["databasePath"]};
    pqxx::work clear_trans(cx);
    std::string schema = optionConfig_["schemaName"];
    clear_trans.exec("SET search_path TO " + clear_trans.quote_name(schema) + ", public");

    clear_trans.exec("TRUNCATE TABLE dataset, email, emailheaderkey, emailheaderval, emailpart, emailpartheaderkey, emailpartheaderval, attributebag RESTART IDENTITY");
    clear_trans.commit();
    LOG_INFO << "Database cleared.";
}


int PostgresqlSaver::getOrCreateDataset(pqxx::work& trans) {
    std::string datasetName = optionConfig_["datasetName"];
    std::string datasetDescription = optionConfig_["datasetDescription"];
    pqxx::result res = trans.exec("SELECT datasetid FROM dataset WHERE datasetname = $1", pqxx::params(datasetName));
    if (res.empty()) {
        res = trans.exec("INSERT INTO dataset (lastupdateddatetime, datasetname, datasetdescription) VALUES (CURRENT_TIMESTAMP, $1, $2) RETURNING datasetid",
            {datasetName, datasetDescription}
        );
    }
    return res[0][0].as<int>();
}

int PostgresqlSaver::addEmail(pqxx::work & trans, int datasetid, const Email &email) {
    pqxx::result res = trans.exec(
        "INSERT INTO email (datasetid, fileidentifier, filedatetime, ismimemultipart) "
        "VALUES ($1, $2, CURRENT_TIMESTAMP, $3) RETURNING emailid",
        {datasetid, email.getAttributeValue("File identifier")->toString(), email.getIsMIMEMultipart()}
    );
    return res[0][0].as<int>();
}

int PostgresqlSaver::addHeaderKey(pqxx::work& trans, int emailid, const std::string& key) {
    pqxx::result res = trans.exec(
        "INSERT INTO emailheaderkey (emailid, headerkey) VALUES ($1, $2) RETURNING emailheaderkeyid",
        {emailid, key}
    );
    return res[0][0].as<int>();
}

void PostgresqlSaver::addHeaderValue(pqxx::work& trans, int headerkeyid, const std::string& value) {
    pqxx::result res = trans.exec(
        "INSERT INTO emailheaderval (headerkeyid, headerval) VALUES ($1, $2)",
        {headerkeyid, value}
    );
}

int PostgresqlSaver::addEmailPart(pqxx::work& trans, int emailid, const std::string& partBody) {
    pqxx::result res = trans.exec(
        "INSERT INTO emailpart (emailid, partbody) VALUES ($1, $2) RETURNING emailpartid",
        {emailid, pqxx::binary_cast(partBody)}
    );
    return res[0][0].as<int>();
}

int PostgresqlSaver::addEmailPartHeaderKey(pqxx::work& trans, int emailpartid, const std::string& key) {
    pqxx::result res = trans.exec(
        "INSERT INTO emailpartheaderkey (emailpartid, headerkey) VALUES ($1, $2) RETURNING emailpartheaderkeyid",
        {emailpartid, key}
    );
    return res[0][0].as<int>();
}

void PostgresqlSaver::addEmailPartHeaderValue(pqxx::work& trans, int emailpartheaderkeyid, const std::string& value) {
    pqxx::result res = trans.exec(
        "INSERT INTO emailpartheaderval (emailpartheaderkeyid, headerval) VALUES ($1, $2)",
        {emailpartheaderkeyid, value}
    );
}

void PostgresqlSaver::addAttribute(pqxx::work& trans, int emailid, const std::string& attributekey, const std::string& attributeval) {
    pqxx::result res = trans.exec(
        "INSERT INTO attributebag (emailid, attributekey, attributeval, datemodified) VALUES ($1, $2, $3::bytea, CURRENT_TIMESTAMP)",
        {emailid, attributekey, pqxx::binary_cast(attributeval)}
    );
}

bool PostgresqlSaver::execute(EmailListView * emailList) {
    LOG_INFO << "Writing to database";
    SET_PLUGIN_STATE("RUNNING");
    try {
        clearDatabase();
        pqxx::connection cx{optionConfig_["databasePath"]};
        pqxx::work insert_trans(cx);
        std::string schema = optionConfig_["schemaName"];
        insert_trans.exec("SET search_path TO " + insert_trans.quote_name(schema) + ", public");
        int datasetid = getOrCreateDataset(insert_trans);

        for (Email& email : *emailList) {

            // Add email, get email ID
            int emailid = addEmail(insert_trans, datasetid, email);

            // For each header
            for (auto& header : email.getHeader()) {
                // Add key, get key ID
                int emailheaderkeyid = addHeaderKey(insert_trans, emailid, header.first);

                // For each header value
                addHeaderValue(insert_trans, emailheaderkeyid, header.second);
            }

            EmailBody* body = email.getBody();

            if (StandardEmailBody* standardBody = dynamic_cast<StandardEmailBody*>(body)) {
                // Handle StandardEmailBody-specific functionality
                addEmailPart(insert_trans, emailid, standardBody->getAllBodyData());
                //LOG_WARNING << "STANDARD";

            } else if (MIMEMultipartBodies* mimeBody = dynamic_cast<MIMEMultipartBodies*>(body)) {
                // Handle MIMEMultipartBody-specific functionality
                for (MIMEMultipartPart& multipart : mimeBody->getMultipartParts()) {
                    int emailpartid = addEmailPart(insert_trans, emailid, multipart.getBody());
                    for (auto& header : multipart.getHeader()) {
                        int emailparthearderkeyid = addEmailPartHeaderKey(insert_trans, emailpartid, header.first);
                        for (auto& headerval : header.second) {
                            addEmailPartHeaderValue(insert_trans, emailparthearderkeyid, headerval);
                        }
                    }
                }
                //LOG_WARNING << "MIME";
            } else {
                SET_PLUGIN_STATE("FAILED");
                LOG_ERROR << "Unknown EmailBody type";
            }
            for (std::string& attributeKey : email.getAttributeKeys()) {
                addAttribute(insert_trans, emailid, attributeKey, email.getAttributeValue(attributeKey)->serializeToString());
            };
        }

        insert_trans.commit();
        LOG_INFO << "Data successfully inserted into the database.";

    }
    catch (std::exception& e) {
        LOG_ERROR << "Exception occurred during database operation: " << e.what();
        SET_PLUGIN_STATE("FAILED");
        return false;
    }
    SET_PLUGIN_STATE("COMPLETE");
    return true;
}


