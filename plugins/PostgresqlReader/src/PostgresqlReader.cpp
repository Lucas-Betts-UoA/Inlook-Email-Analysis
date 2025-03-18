#include "PostgresqlReader.hpp"
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
#include "EmailBody.hpp"
#include <pqxx/pqxx>
#include "PluginRegistry.hpp"

// Constructor
PostgresqlReader::PostgresqlReader(const std::string& instanceID) : PluginRunnableInterface(instanceID) {
    pluginName_ = "PostgresqlReader";
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
    inputAttributes_ = {};
    generatedAttributes_ = {};
    SET_PLUGIN_STATE("LOADED");
}

// Destructor
PostgresqlReader::~PostgresqlReader() = default;

bool PostgresqlReader::instantiateRecursive() {
    SET_PLUGIN_STATE("READY");
    return true;
}

nlohmann::json PostgresqlReader::printRecursiveInstanceTreeJson() {
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

void PostgresqlReader::getEmails(pqxx::result &result, pqxx::work& trans, EmailListView *emailList) {
    for (const auto& emailRow : result) {
        int emailId = emailRow[0].as<int>();
        auto fileIdentifier = emailRow[1].as<std::string>();
        bool isMimeMultipart = emailRow[2].as<bool>();

        Email newEmail; // Assuming Email constructor takes these parameters
        newEmail.setIsMIMEMultipart(isMimeMultipart);

        // Query to get headers for the current email
        pqxx::result headers = trans.exec("SELECT emailheaderkeyid, headerkey FROM emailheaderkey WHERE emailid = $1", pqxx::params(emailId));

        for (const auto& headerRow : headers) {
            int emailheaderkeyid = headerRow[0].as<int>();
            std::string headerKey = headerRow[1].as<std::string>();

            // Query to get values for each header key
            pqxx::result values = trans.exec("SELECT headerval FROM emailheaderval WHERE headerkeyid = $1",
                                              pqxx::params(emailheaderkeyid));

            for (const auto& valueRow : values) {
                std::string headerValue = valueRow[0].as<std::string>();
                newEmail.setHeader(headerKey, headerValue); // Assuming addHeader method exists in Email class
            }
        }
        try {
            pqxx::result attributes = trans.exec("SELECT attributekey, convert_from(attributeval, 'UTF-8') FROM attributebag WHERE emailid = $1", pqxx::params(emailId));
            for (const auto& attributeRow : attributes) {
                std::string key = attributeRow[0].as<std::string>();
                std::string value = attributeRow[1].as<std::string>();
                newEmail.insertAttribute(key, AttributeBagRegistry::deserializeAttribute(value));
            }
        } catch (const std::exception& e) {
            LOG_ERROR << "PostgresqlReader exception: " << e.what();
        }



        std::unique_ptr<EmailBody> partBodies;

        // Query to get parts for the current email if it's multipart
        if (isMimeMultipart) {
            pqxx::result parts = trans.exec("SELECT emailpartid, partbody FROM emailpart WHERE emailid = $1", pqxx::params(emailId));
            partBodies = std::make_unique<MIMEMultipartBodies>();
            for (const auto& partRow : parts) {
                int partId = partRow[0].as<int>();
                std::string partBody = partRow[1].as<std::string>();

                pqxx::result mimeHeaders = trans.exec("SELECT emailpartheaderkeyid, headerkey FROM emailpartheaderkey WHERE emailpartid = $1", pqxx::params(partId));
                std::pmr::map<std::string, std::vector<std::string>> headerMap;

                for (const auto& headerRow : mimeHeaders) {
                    int emailpartheaderkeyid = headerRow[0].as<int>();
                    std::string headerKey = headerRow[1].as<std::string>();

                    pqxx::result mimeValues = trans.exec("SELECT headerval FROM emailpartheaderval WHERE emailpartheaderkeyid = $1", pqxx::params(emailpartheaderkeyid));
                    std::vector<std::string> headerValues;
                    for (const auto& valueRow : mimeValues) {
                        std::string headerValue = valueRow[0].as<std::string>();
                        headerValues.push_back(headerValue);
                    }
                    headerMap[headerKey] = headerValues;
                }

                pqxx::result convertedBody;
                try {
                    convertedBody = trans.exec("SELECT convert_from($1::bytea, $2)", pqxx::params(partBody, "UTF-8"));
                } catch (const std::exception& e) {
                    LOG_ERROR << "Error converting body: " << e.what();
                    LOG_ERROR << "Skipping email.";
                    continue;
                }
                std::string encodedPartBody = convertedBody[0][0].as<std::string>();

                dynamic_cast<MIMEMultipartBodies*>(partBodies.get())->addPart(headerMap, encodedPartBody);
            }
            newEmail.setBody(std::move(partBodies));
        } else {
            pqxx::result standardBody = trans.exec("SELECT partbody FROM emailpart WHERE emailid = $1", pqxx::params(emailId));
            partBodies = std::make_unique<StandardEmailBody>();
            auto body = standardBody[0][0].as<std::string>();

            pqxx::result convertedBody;
            try {
                convertedBody = trans.exec("SELECT convert_from($1::bytea, $2)", pqxx::params(body, "utf-8"));
            } catch (const std::exception& e) {
                LOG_ERROR << "Error converting body: " << e.what();
                LOG_ERROR << "Skipping email.";
                continue;
            }
            std::string encodedBody = convertedBody[0][0].as<std::string>();
            dynamic_cast<StandardEmailBody*>(partBodies.get())->setContent(encodedBody);
            newEmail.setBody(std::move(partBodies));
        }
        bool isUnique = true;
        newEmail.generateUniqueHash();
        for (const auto& email : *emailList) {
            if (email.getUniqueHash() == newEmail.getUniqueHash()) {
                LOG_INFO << "Email already exists: " << email.getUniqueHash();
                isUnique = false;
            }
        }
        if (isUnique) {
            LOG_INFO << "Email doesn't exist: " << newEmail.getUniqueHash() << ", file: " << (newEmail.getAttributeValue("File identifier")->toString());
            emailList->insertEmail(newEmail);
        }

    }
}

bool PostgresqlReader::execute(EmailListView * emailList) {
    LOG_INFO << "Reading from database.";
    SET_PLUGIN_STATE("RUNNING");
    try {
        pqxx::connection cx{optionConfig_["databasePath"]};
        pqxx::work trans(cx);
        std::string schema = optionConfig_["schemaName"];
        trans.exec("SET search_path TO " + trans.quote_name(schema) + ", public");

        // Query to get all datasets
        pqxx::result dataset = trans.exec("SELECT datasetid FROM dataset WHERE datasetname = $1", pqxx::params(optionConfig_["datasetName"].get<std::string>()));

        int datasetid = dataset[0][0].as<int>();
        if (optionConfig_["filters"].empty()) {
            pqxx::result emails = trans.exec("SELECT emailid, fileidentifier, ismimemultipart FROM email WHERE datasetid = $1", pqxx::params(datasetid));
            getEmails(emails, trans, emailList);
        } else {
            std::string query = "SELECT emailid, fileidentifier, ismimemultipart FROM email WHERE datasetid = $1";
            pqxx::params values;
            values.append(datasetid);

            for (const auto& filter : optionConfig_["filters"]) {
                query += " AND " + trans.quote_name(filter["columnName"].get<std::string>()) +
                         " " + filter["condition"].get<std::string>() + " $" + std::to_string(values.size() + 1);
                values.append(filter["value"].get<std::string>());
            }

            pqxx::result emails = trans.exec(query, values);
            getEmails(emails, trans, emailList);
        }
        LOG_INFO << "Emails successfully read from the database.";
    } catch (const std::exception &e) {
        LOG_ERROR << "Exception occurred during database read operation: " << e.what();
        SET_PLUGIN_STATE("FAILED");
        return false;
    }
    SET_PLUGIN_STATE("COMPLETE");
    return true;
}
