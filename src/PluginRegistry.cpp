#include "PluginRegistry.hpp"
#include "PluginInterface.hpp"
#include "PluginExecutorInterface.hpp"
#include "RootPluginExecutor.hpp"

static std::atomic pluginCounter{0};

PluginRegistry *PluginRegistry::getInstance() {
    static PluginRegistry instance;
    return &instance;
}

std::vector<std::string> PluginRegistry::listAvailablePlugins() {
    auto key_selector = [](auto pair){return pair.first;}; // lambda for getting the first of a pair
    std::vector<std::string> handles(pluginRecords_.size()); // returnable key list
    std::ranges::transform(pluginRecords_, handles.begin(), key_selector); // transform pluginHandles_ string key to vector of strings.
    return handles;
}

nlohmann::json PluginRegistry::listImplementedInterfaces(const std::string& pluginName) {
    nlohmann::json result = pluginRecords_[pluginName].interfaces_implemented;
    return result;
}

nlohmann::json PluginRegistry::listInstances() {
    nlohmann::json instanceIDs;
    for (const auto &pluginRecord: pluginRecords_) {
        for (const auto &instanceID: pluginRecord.second.instances) {
            instanceIDs[instanceID.first] = {
                {"state", instanceID.second.lock()->getState()},
                {"pluginName", instanceID.second.lock()->getPluginName()}
            };
        }
    }
    return instanceIDs;
}

std::vector<std::string> PluginRegistry::listInstancesByCreateFunc(const std::string &pluginCreateFuncName) {
    std::vector<std::string> instanceIDs;
    if (const auto it = pluginRecords_.find(pluginCreateFuncName); it != pluginRecords_.end()) {
        for (const auto &instanceID: it->second.instances | std::views::keys) {
            instanceIDs.push_back(instanceID);
        }
    }
    return instanceIDs;
}

// Can rely on unique instance names as they increment on instance creation.
std::optional<std::string> PluginRegistry::getCreateFuncForInstance(const std::string &instanceName) const {
    // Iterate over all plugin records.
    for (const auto &[first, second] : pluginRecords_) {
        // first is the plugin's create function key.
        // second.instances maps instance IDs to shared_ptr<PluginInterface>.
        if (second.instances.contains(instanceName)) {
            return first;
        }
    }
    return std::nullopt;
}


bool PluginRegistry::loadAllPlugins() {
    std::string pluginsDir = "./plugins/";

    if (!std::filesystem::exists(pluginsDir) || !std::filesystem::is_directory(pluginsDir)) {
        LOG_ERROR << "Plugins directory not found: " << pluginsDir;
        return false;
    }

    for (const auto& entry : std::filesystem::directory_iterator(pluginsDir)) {
        if (entry.is_directory()) {
            std::string pluginName = entry.path().filename().string();
            if (!loadPluginCreateFunc(pluginName)) {
                LOG_ERROR << "Failed to load plugin: " << pluginName;
            } else {
                LOG_DEBUG_VERBOSE << "Successfully loaded plugin: " << pluginName;
            }
        }
    }
    return true;
}

bool PluginRegistry::loadPluginCreateFunc(const std::string &pluginName) {
    if (pluginRecords_.contains(pluginName)) {
        LOG_DEBUG_VERBOSE << "CreateFunc/Plugin for " << pluginName << " already loaded.";
        return true;
    }

    std::string pluginPath = "./plugins/" + pluginName + "/";
    std::string pluginSuffix;
#ifdef _WIN32
    pluginSuffix += ".dll";
#elif __APPLE__
    pluginPath += "lib";
    pluginSuffix += ".dylib";
#elif __linux__
    pluginPath += "lib";
    pluginSuffix += ".so";
#endif
    pluginPath += pluginName;
    pluginPath += pluginSuffix;

    void* handle = dlopen(pluginPath.c_str(), RTLD_NOW);
    if (!handle) {
        LOG_ERROR << "Error loading plugin: " << dlerror();
        return false;
    }

    pluginRecords_[pluginName].handle = handle;
    return true;
}

std::optional<std::pair<std::string, std::shared_ptr<PluginInterface>>>
PluginRegistry::createPluginInstance(const std::string &pluginCreateFuncName, const nlohmann::json &options) {
    if (!pluginRecords_.contains(pluginCreateFuncName)) {
         if (!loadPluginCreateFunc(pluginCreateFuncName)) { // loads, and confirms it loaded. False if couldn't load
             LOG_ERROR << "Error loading plugin " << pluginCreateFuncName << " plugin.";
         }
    } else {
        LOG_DEBUG_VERBOSE << "pluginCreateFunc already loaded for " << pluginCreateFuncName;
    }

    try {
        CreateFunc createPlugin = pluginRecords_[pluginCreateFuncName].createFunc;

        std::string pluginInstanceID = pluginCreateFuncName + "_" + std::to_string(pluginCounter++);

        std::shared_ptr<PluginInterface> instance(
            createPlugin(pluginInstanceID),
            [this, pluginCreateFuncName, pluginInstanceID](PluginInterface* p)
        {
            LOG_DEBUG_VERBOSE << "Destroying instance of " << pluginCreateFuncName;
            pluginRecords_[pluginCreateFuncName].instances.erase(pluginInstanceID);
        });

        instance->setConfig(options);
        pluginRecords_[pluginCreateFuncName].instances[pluginInstanceID] = instance;
        return std::make_pair(pluginInstanceID, std::move(instance));
    } catch (std::exception &e) {
        LOG_ERROR << "Error creating plugin instance: " << e.what();
        return std::nullopt;
    }
}

//unused for now.
bool PluginRegistry::unloadPluginHandle(const std::string &pluginCreateFuncName) {
    auto it = pluginRecords_.find(pluginCreateFuncName);
    if (it == pluginRecords_.end()) {
        LOG_WARNING << "Plugin " << pluginCreateFuncName << " is not loaded.";
        return false;
    }

    // Check if any instances still exist
    if (!pluginRecords_[pluginCreateFuncName].instances.empty()) {
        LOG_WARNING << "Tried to unload " << pluginCreateFuncName << ", but instances are still active.";
        return false;
    }
    pluginRecords_.erase(it);
    LOG_DEBUG_VERBOSE << "Unloaded plugin: " << pluginCreateFuncName;

    return true;
}

bool PluginRegistry::insertInterfacesImplemented(const std::string& pluginName, const std::vector<std::string>& interfaces) {
    if (pluginRecords_.contains(pluginName)) {
        pluginRecords_[pluginName].interfaces_implemented.insert(std::end(pluginRecords_[pluginName].interfaces_implemented), std::begin(interfaces), std::end(interfaces));
        return true;
    }
    LOG_WARNING << "Plugin " << pluginName << " is seemingly not loaded.";
    return false;
}

void PluginRegistry::registerPlugin(const std::string& pluginCreateFuncName, PluginInterface* (*createFunc)(const std::string&)) {
    pluginRecords_.emplace(pluginCreateFuncName, PluginRecord{ createFunc, nullptr,  {}, {}});
}