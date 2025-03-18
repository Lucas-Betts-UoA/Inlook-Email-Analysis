#include "EmailListFilter.hpp"
#include "Logger.hpp"
#include "EmailListView.hpp"
#include <PluginInterface.hpp>
#include <unordered_map>
#include <iostream>
#include <functional>
#include <string>
#include <regex>
#include "Email.hpp"
#include <vector>
#include <filesystem>

EmailListFilter::EmailListFilter(const std::string& instanceID) : PluginRunnableInterface(instanceID) {
    pluginName_ = "EmailListFilter";
    instanceID_ = instanceID;
    optionSchema_ = R"(
{
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "type": "object",
    "properties": {
      "filters": {
        "type": "array",
        "items": {
          "type": "object",
          "properties": {
            "fields": {
              "type": "array",
              "items": {
                "type": "object",
                "properties": {
                  "value": {
                    "type": "string",
                    "description": "The value of the field."
                  },
                  "outcome": {
                    "type": "string",
                    "enum": ["include", "exclude"],
                    "description": "The outcome of the filter (include or exclude)."
                  },
                  "filterBy": {
                    "type": "string",
                    "description": "The method to filter by."
                  },
                  "filterVals": {
                    "type": "array",
                    "items": {
                      "type": "object",
                      "properties": {
                        "filterValue": {
                          "type": "string",
                          "description": "The value to filter by."
                        }
                      },
                      "required":["value"],
                      "additionalProperties": false
                    },
                    "description": "A list of values to filter by."
                  }
                },
                "required": [
                    "value",
                    "outcome",
                    "filterBy",
                    "filterVals"
                ],
                "additionalProperties": false
              },
              "description": "A list of fields to consider for filtering."
            }
          },
          "required": [
            "fields"
          ],
          "additionalProperties": false
        },
        "description": "A list of filters to apply."
      }
    },
    "required": [
      "filters"
    ],
    "additionalProperties": false
  }
    )"_json;
    inputAttributes_ = {};
    generatedAttributes_ = {};
    SET_PLUGIN_STATE("LOADED");
}

EmailListFilter::~EmailListFilter() = default;

bool EmailListFilter::instantiateRecursive() {
    SET_PLUGIN_STATE("READY");
    return true;
}

nlohmann::json EmailListFilter::printRecursiveInstanceTreeJson() {
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

// Helper function to process a single email
void EmailListFilter::processEmail(const Email& email, EmailListView* emailList, const filterStruct& emailFilter) const {
    bool matchFound = false;
    for (const auto& filterValue : emailFilter.values) {
        if (emailFilter.filterBy == "string") {
            if (std::ranges::find(filters.at(emailFilter.field), filterValue) != filters.at(emailFilter.field).end()) {
                matchFound = true;
            }
        } else {
            std::regex re(filterValue);
            const auto it = std::ranges::find_if(filters.at(emailFilter.field), [&](const auto& e) { return std::regex_match(e, re); });
            if (it != filters.at(emailFilter.field).end()) {
                matchFound = true;
            }
        }
    }
    // Remove email based on the outcome and whether a match was found
    if ((emailFilter.outcome == "include" && !matchFound) ||
        (emailFilter.outcome == "exclude" && matchFound)) {
        //emailList->removeEmail(email);
    }
}

// Main execution logic
bool EmailListFilter::execute(EmailListView* emailList) {
    LOG_INFO << "EmailListFilter::execute called.";
    SET_PLUGIN_STATE("RUNNING");
    for (auto it = emailList->begin(); it != emailList->end(); ++it) {
        Email& email = *it;
        filters["headerKey"] = email.getHeaderKeys();
        filters["headerVal"] = email.getHeaderValues();
        filters["attributeKey"] = email.getAttributeKeys();
        std::vector<std::string> values;
        for (const auto& val : email.getAttributeValues()) {
            values.push_back(val->toString());
        }
        filters["attributeVal"] = values;
        std::vector<std::string> bodys;
        EmailBody* body = email.getBody();
        if (StandardEmailBody* standardBody = dynamic_cast<StandardEmailBody*>(body)) {
            bodys.push_back(standardBody->getAllBodyData());
            filters["body"] = bodys;
        } else if (const MIMEMultipartBodies* mimeBody = dynamic_cast<const MIMEMultipartBodies*>(body)) {
            for (const MIMEMultipartPart& multipart : mimeBody->getMultipartParts()) {
                bodys.push_back(multipart.getBody());
                filters["MIMEPartKey"] = multipart.getHeaderKeys();
                filters["MIMEPartVal"] = multipart.getHeaderValues();
            }
            filters["body"] = bodys;

        }
        for (const auto& filter : optionConfig_["filters"]) {
            for (const auto& field : filter["fields"]) {
                filterStruct filtering;
                filtering.field = field["value"].get<std::string>();
                filtering.outcome = field["outcome"].get<std::string>();
                filtering.filterBy = field["filterBy"].get<std::string>();
                std::vector<std::string> filterValues;
                for (const auto& vals : field["filterVals"]) {
                    filterValues.push_back(vals["filterValue"].get<std::string>());
                }
                filtering.values = filterValues;
                processEmail(email, emailList, filtering);
            }
        }
    }
    SET_PLUGIN_STATE("COMPLETE");
    return true;
}