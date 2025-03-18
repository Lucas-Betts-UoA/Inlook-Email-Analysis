#include "PluginInterface.hpp"
#include <sstream>
#include <exception>
#include "EmailListView.hpp"
#include <string>
#include <vector>
#include "Logger.hpp"
#include "PluginRegistry.hpp"
#include "nlohmann/json.hpp"
#include "nlohmann/json-schema.hpp"

std::string PluginInterface::getPluginName() {
    return pluginName_;
}

nlohmann::json &PluginInterface::getSchema() {
    return optionConfig_;
}

std::string PluginInterface::getInstanceID() const {
    return instanceID_;
}

bool PluginInterface::setConfig(const nlohmann::json &pluginConfig) {
    try {
        this->optionConfig_ = pluginConfig;
    } catch (const std::exception& e) {
        LOG_ERROR << "Failed to set plugin config: " << e.what();
        return false;
    }
    return true;
}

nlohmann::json PluginInterface::getConfig() {
    return optionConfig_;
}

std::vector<std::string> PluginInterface::getInputAttributes() const {
    return inputAttributes_;
}

std::vector<std::string> PluginInterface::getOutputAttributes() const {
    std::vector<std::string> combinedAttributes = generatedAttributes_;
    combinedAttributes.insert(combinedAttributes.end(), inputAttributes_.begin(), inputAttributes_.end());
    return combinedAttributes;
}

std::string PluginInterface::getState() const {
    return stateManager_.getState();
}

void PluginInterface::CustomErrorHandler::error(const nlohmann::json_pointer<std::string>& pointer,
                                                  const nlohmann::json& instance,
                                                  const std::string& message) {
    basic_error_handler::error(pointer, instance, message);
    LOG_ERROR << "Schema validation error: [" << pointer << "] "
              << message << " | Instance: " << instance.dump();
}

static void cleanSchema(nlohmann::json &schema) {
    if (schema.is_object()) {
        std::vector<std::string> keysToRemove;

        for (auto& [key, value] : schema.items()) {
            if (key.starts_with("_inlook_")) {
                keysToRemove.push_back(key); // Mark for removal
            } else {
                cleanSchema(value); // Recursively clean sub-objects
            }
        }

        for (const auto& key : keysToRemove) {
            schema.erase(key);
        }
    } else if (schema.is_array()) {
        for (auto& item : schema) {
            cleanSchema(item);
        }
    }
}

bool PluginInterface::validateConfig() {
    nlohmann::json_schema::json_validator validator;
    CustomErrorHandler errHandler;

    try { // Schema validity check
        cleanSchema(getSchema()); // Remove non-standard fields
        validator.set_root_schema(getSchema());
    } catch (const std::exception& e) {
        LOG_WARNING << "Schema invalid: " << e.what();
        SET_PLUGIN_STATE("FAILED");
        return false;
    }

    try { // Config check against schema.
        validator.validate(optionConfig_, errHandler);
    } catch (const std::exception& e) {
        LOG_ERROR << "Validation exception: " << e.what();
        SET_PLUGIN_STATE("FAILED");
        return false;
    }

    if (errHandler) {
        LOG_ERROR << "Schema validation failed.";
        SET_PLUGIN_STATE("FAILED");
        return false;
    }

    // LOG_DEBUG_VERBOSE << "Schema validation succeeded.";
    return true;
}