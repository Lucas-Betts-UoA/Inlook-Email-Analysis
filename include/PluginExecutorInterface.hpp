#pragma once
#include <string>
#include <vector>
#include "PluginInterface.hpp"
#include "nlohmann/json.hpp"

/**
 * @brief Abstract interface for plugin executors.
 *
 * PluginExecutorInterface extends PluginInterface with methods for managing
 * multiple plugin instances, executing them, and handling instance CRUD and state.
 * Derived classes must implement all pure virtual methods.
 */
class PluginExecutorInterface : public PluginInterface {
public:
    /**
     * @brief Virtual destructor.
     *
     */
    ~PluginExecutorInterface() override = default;

    /**
     * @brief Clears all managed plugin instances.
     *
     * Removes and cleans up all plugin instances managed by this executor.
     */
    virtual void clearAllInstances() = 0;

    /**
     * @brief Executes just one plugin by ID in the executor interface.
     *
     * Removes and cleans up all plugin instances managed by this executor.
     */
    virtual bool executeOne(EmailListView *, std::string) = 0;

    /**
     * @brief Retrieves a specific instantiated plugin by instance ID.
     *
     * @param pluginInstanceID The unique identifier for the plugin instance.
     * @return A pointer to the plugin instance, or nullptr if not found.
     */
    virtual std::shared_ptr<PluginInterface> getInstantiatedPluginByID(const std::string& pluginInstanceID) = 0;

    /**
     * @brief Updates the configuration of a specific plugin instance.
     *
     * @param pluginInstanceID The unique identifier for the plugin instance.
     * @param newOptions The new configuration options as a JSON object.
     */
    virtual void updateInstantiatedPluginConfig(const std::string& pluginInstanceID, const nlohmann::json& newOptions) = 0;

    /**
     * @brief Removes a specific plugin instance.
     *
     * @param pluginInstanceID The unique identifier for the plugin instance.
     */
    virtual void removeInstantiatedPlugin(const std::string& pluginInstanceID) = 0;

    /**
     * @brief Retrieves the configuration of a specific plugin instance.
     *
     * @param pluginInstanceID The unique identifier for the plugin instance.
     * @return The configuration as a string.
     */
    virtual std::string getInstantiatedPluginConfig(const std::string& pluginInstanceID) = 0;

    /**
     * @brief Reloads plugins from a configuration.
     *
     * Validates the new configuration and reloads plugin instances accordingly.
     *
     * @return True if the reload succeeded, false otherwise.
     */
    virtual bool reloadPluginsFromConfig() = 0;

    /**
     * @brief Retrieves the status of a specific plugin instance.
     *
     * @param pluginInstanceID The unique identifier for the plugin instance.
     * @return The current status of the plugin as a string.
     */
    virtual std::string getInstantiatedPluginStatus(const std::string &pluginInstanceID) = 0;

    /**
     * @brief Lists the instance IDs of all managed plugin instances.
     *
     * @return A vector containing the instance IDs.
     */
    virtual std::vector<std::string> listPluginsByID() = 0;

protected:
    /**
     * @brief Protected constructor.
     *
     * Only derived classes may instantiate PluginExecutorInterface.
     */
    explicit PluginExecutorInterface(const std::string& name) : PluginInterface(name) {}
};