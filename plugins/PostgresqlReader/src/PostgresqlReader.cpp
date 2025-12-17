#include "PostgresqlReader.hpp"
#include "PluginInterface.hpp"
#include "Logger.hpp"
#include "EmailListView.hpp"
#include <string>
#include <regex>
#include "Email.hpp"
#include <vector>
#include "EmailBody.hpp"
#include <pqxx/pqxx>
#include "PluginRegistry.hpp"
#include <unicode/uloc.h>
#include <unicode/ustring.h>

#include "../../EmailLoader/include/EmailLoaderAttributes.hpp"

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

std::string PostgresqlReader::convertToUTF8(const std::vector<char>& inputBuffer, const std::string& encoding = "UTF-8") {
    UErrorCode status = U_ZERO_ERROR;

    // Open ICU converter for the detected encoding
    UConverter* conv = ucnv_open(encoding.c_str(), &status);
    if (U_FAILURE(status)) {
        throw std::runtime_error("Error: Unable to open ICU converter for " + encoding);
    }

    // Step 1: Calculate required UTF-8 buffer size (nullptr = no conversion)
    status = U_ZERO_ERROR;
    int32_t requiredSize = ucnv_convert("UTF-8", encoding.c_str(), nullptr, 0,
                                        inputBuffer.data(), inputBuffer.size(), &status);
    //LOG_DEBUG_VERBOSE << "Required output buffer size: " << requiredSize;
    if (status != U_BUFFER_OVERFLOW_ERROR) {
        ucnv_close(conv);
        throw std::runtime_error("Error: Failed to calculate buffer size for UTF-8 conversion.");
    }

    // Step 2: Allocate buffer of the correct size
    status = U_ZERO_ERROR;
    std::vector<char> utf8Buffer(requiredSize);

    // Step 3: Perform the actual conversion (with correctly sized target buffer)
    int32_t actualSize = ucnv_convert("UTF-8", encoding.c_str(), utf8Buffer.data(), utf8Buffer.size(),
                                      inputBuffer.data(), inputBuffer.size(), &status);

    // Cleanup
    ucnv_close(conv);

    if (U_FAILURE(status)) {
        throw std::runtime_error("Error: Conversion to UTF-8 failed.");
    }

    return std::string(utf8Buffer.begin(), utf8Buffer.begin() + actualSize);
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
                newEmail.setHeader(headerKey, valueRow[0].as<std::string>()); // Assuming addHeader method exists in Email class
            }
        }
        try {
            pqxx::result attributes = trans.exec("SELECT attributekey, attributeval FROM attributebag WHERE emailid = $1", pqxx::params(emailId));
            for (const auto& attributeRow : attributes) {
                std::string key = attributeRow[0].as<std::string>();
                pqxx::binarystring rawAttributeValue(attributeRow[1]);
                std::vector<char> rawValueBytes(rawAttributeValue.begin(), rawAttributeValue.end());

                if (key == "File Bytes") {
                    std::string encoding = newEmail.getAttributeValue("Encoding")->toString();
                    size_t pos = encoding.find(',');
                    if (pos == std::string::npos) throw std::runtime_error("Invalid pair format.");
                    std::string encodedFileBytes = convertToUTF8(rawValueBytes, encoding.substr(1, pos-1));
                    newEmail.insertAttribute(key, AttributeBagRegistry::deserializeAttribute(encodedFileBytes));
                } else {
                    std::string encodedAttributeValue = convertToUTF8(rawValueBytes, "UTF-8");
                    newEmail.insertAttribute(key, AttributeBagRegistry::deserializeAttribute(encodedAttributeValue));
                }
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
                pqxx::binarystring partBody(partRow[1]);
                const std::vector<char> rawPartBody(partBody.begin(), partBody.end());

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

                std::string encoding = newEmail.getAttributeValue("Encoding")->toString();
                size_t pos = encoding.find(',');
                if (pos == std::string::npos) throw std::runtime_error("Invalid pair format.");

                //LOG_DEBUG_VERBOSE << "Converting MIME body";
                std::string encodedPartBody = convertToUTF8(rawPartBody, encoding.substr(1, pos-1));
                dynamic_cast<MIMEMultipartBodies*>(partBodies.get())->addPart(headerMap, encodedPartBody);
            }
            newEmail.setBody(std::move(partBodies));
        } else {
            pqxx::result standardBody = trans.exec("SELECT partbody FROM emailpart WHERE emailid = $1", pqxx::params(emailId));
            partBodies = std::make_unique<StandardEmailBody>();
            pqxx::binarystring body(standardBody[0][0]);
            const std::vector<char> rawBody(body.begin(), body.end());

            std::string encoding = newEmail.getAttributeValue("Encoding")->toString();
            size_t pos = encoding.find(',');
            if (pos == std::string::npos) throw std::runtime_error("Invalid pair format.");

            //LOG_DEBUG_VERBOSE << "Converting standard body";
            std::string encodedBody = convertToUTF8(rawBody, encoding.substr(1, pos-1));
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
