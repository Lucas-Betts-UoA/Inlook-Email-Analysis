#pragma once

#include "nlohmann/json.hpp"
#include <mutex>
#include <string>
#include "EmailListView.hpp"
#include "PluginInterface.hpp"
#include "EmailStorage.hpp"

/**
 * @brief GlobalConfigManager is a singleton that manages the application's global configuration.
 *
 * It loads, stores, and saves configuration data as JSON and provides thread-safe access.
 * Other components can use this manager to get and set configuration values.
 */
class GlobalConfigManager {
public:
    /**
     * @brief Gets the singleton instance of GlobalConfigManager.
     * @return A reference to the singleton instance.
     */
    static GlobalConfigManager *getInstance();

    bool isReady() const;

    void initialize_root_plugin_instance();

    void clear_root_plugin_instance() const;

    /**
     * @brief Loads the configuration from a file.
     *
     * @return True if the configuration was loaded successfully, false otherwise.
     */
    bool loadConfig();

    /**
     * @brief Saves the current configuration to a file.
     *
     * @return True if the configuration was saved successfully, false otherwise.
     */
    bool saveConfig() const;

    bool saveWorkflow() const;

    /**
     * @brief Retrieves the entire global configuration as a JSON object.
     *
     * @return A JSON object containing the global configuration.
     */
    nlohmann::json getGlobalConfig() const;
    /**
 * @brief Retrieves the current workflow as a JSON object.
 *
 * @return A JSON object containing the current workflow.
 */
    nlohmann::json getCurrentWorkflow() const;

    /**
     * @brief Sets the global config filename
     *
     * @param config_filename  The filename for the config to be set.
     */
    void setConfig(const std::string& config_filename);

    /**
     * @brief Retrieves the entire global configuration as a JSON object.
     *
     * @return A JSON object containing the global configuration.
     */
    EmailStorage *getEmailListPointer() const;

    nlohmann::json listWorkflowJsonFiles() {
        nlohmann::json workflowFiles = nlohmann::json::array();
        for (const auto& entry : std::filesystem::directory_iterator(WORKFLOWS_DIR)) {
            if (entry.is_regular_file() && entry.path().extension() == ".json") {
                workflowFiles.push_back(entry.path().filename().string());
            }
        }
        return workflowFiles;
    }

    bool setActiveWorkflowFile(const std::string& workflowFilename);

    std::string getActiveWorkflowFileName() const;

    bool createNewWorkflowFile(const std::string& workflowFilename);

    /**
     * @brief Retrieves a configuration value associated with a given key.
     *
     * @tparam T The type of the configuration value.
     * @param key The key of the configuration value.
     * @param defaultValue The default value to return if the key is not found.
     * @return The configuration value, or defaultValue if the key doesn't exist.
     */
    template <typename T>
    T getWorkflowValue(const std::string& key, const T& defaultValue = T()) const {
        std::lock_guard lock(mtx_);
        if (CurrentWorkflow_.contains(key)) {
            return CurrentWorkflow_.at(key).get<T>();
        }
        return defaultValue;
    }

    /**
     * @brief Sets a configuration value for a given key.
     *
     * @tparam T The type of the configuration value.
     * @param key The key to set.
     * @param value The value to set.
     */
    template <typename T>
    void setWorkflowValue(const std::string& key, const T& value) {
        std::lock_guard lock(mtx_);
        CurrentWorkflow_[key] = value;
        if (!getInstance()->saveWorkflow()) LOG_ERROR << "Error saving workflow after value change.";
    }

    /**
     * @brief Retrieves a configuration value associated with a given key.
     *
     * @tparam T The type of the configuration value.
     * @param key The key of the configuration value.
     * @param defaultValue The default value to return if the key is not found.
     * @return The configuration value, or defaultValue if the key doesn't exist.
     */
    template <typename T>
    T getGlobalConfigValue(const std::string& key, const T& defaultValue = T()) const {
        std::lock_guard lock(mtx_);
        if (GlobalConfig_.contains(key)) {
            return GlobalConfig_.at(key).get<T>();
        }
        return defaultValue;
    }

    /**
     * @brief Sets a configuration value for a given key.
     *
     * @tparam T The type of the configuration value.
     * @param key The key to set.
     * @param value The value to set.
     */
    template <typename T>
    void setGlobalConfigValue(const std::string& key, const T& value) {
        std::lock_guard lock(mtx_);
        GlobalConfig_[key] = value;
        if (!getInstance()->saveConfig()) LOG_ERROR << "Error saving global config after value change.";
    }

    PluginInterface *getRootPluginExecutor() const;

    void executeRootAsync();

private:
    GlobalConfigManager() : emailStorage_(new EmailStorage) {}
    ~GlobalConfigManager() = default;

    std::shared_ptr<EmailStorage> emailStorage_;

    mutable std::mutex mtx_;
    std::pair<std::string, std::shared_ptr<PluginInterface>> rootPluginExecutor_;

    std::string GlobalConfigFilename_;
    nlohmann::json GlobalConfig_ = {};

    std::string CurrentWorkflowFilename_;
    nlohmann::json CurrentWorkflow_ = {};

    std::string WORKFLOWS_DIR = "workflows";
};