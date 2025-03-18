#include "GlobalConfigManager.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include "WebUIManager.hpp"
#include "EmailStorage.hpp"
#include "EmailListView.hpp"

GlobalConfigManager* GlobalConfigManager::getInstance() {
    static GlobalConfigManager instance;
    return &instance;
}

bool GlobalConfigManager::isReady() const {
    return (rootPluginExecutor_.second && true);
}

void GlobalConfigManager::initialize_root_plugin_instance() {
    if (!rootPluginExecutor_.second) { // Plugin doesnt exist.
        auto instancePair = Plugins->createPluginInstance("RootPluginExecutor", CurrentWorkflow_);
        if (!instancePair) {
            LOG_ERROR << "Failed to create plugin instance";
            return;
        }
        rootPluginExecutor_ = std::make_pair(instancePair->first, instancePair->second);
    }
}

void GlobalConfigManager::clear_root_plugin_instance() const {
    if (rootPluginExecutor_.second) { // It exists, so we are clearing instances.
        auto* executor = dynamic_cast<PluginExecutorInterface*>(rootPluginExecutor_.second.get());
        if (executor) {
            executor->clearAllInstances();
        }
    }
}

bool GlobalConfigManager::loadConfig() {
    std::lock_guard<std::mutex> lock(mtx_);
    std::ifstream file(GlobalConfigFilename_);
    if (!file.is_open()) {
        return false;
    }
    try {
        file >> GlobalConfig_;
    } catch (const std::exception& e) {
        return false;
    }
    return true;
}

bool GlobalConfigManager::saveConfig() const {
    std::lock_guard<std::mutex> lock(mtx_);
    std::ofstream file(GlobalConfigFilename_);
    if (!file.is_open()) {
        return false;
    }
    try {
        file << GlobalConfig_.dump(4);
    } catch (const std::exception& e) {
        return false;
    }
    return true;
}

bool GlobalConfigManager::saveWorkflow() const {
    std::lock_guard<std::mutex> lock(mtx_);
    std::ofstream file(CurrentWorkflowFilename_);
    if (!file.is_open()) {
        return false;
    }
    try {
        file << CurrentWorkflow_.dump(4);
    } catch (const std::exception& e) {
        return false;
    }
    return true;
}

nlohmann::json GlobalConfigManager::getGlobalConfig() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return GlobalConfig_;
}

nlohmann::json GlobalConfigManager::getCurrentWorkflow() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return CurrentWorkflow_;
}

void GlobalConfigManager::setConfig(const std::string& config_filename) {
    std::lock_guard<std::mutex> lock(mtx_);
    GlobalConfigFilename_ = config_filename;
}

EmailStorage *GlobalConfigManager::getEmailListPointer() const {
    return emailStorage_.get();
}

PluginInterface* GlobalConfigManager::getRootPluginExecutor() const {
    return rootPluginExecutor_.second.get();
}

void GlobalConfigManager::executeRootAsync() {
    if (!rootPluginExecutor_.second) {
        LOG_ERROR << "Root plugin executor is not set.";
        return;
    }

    if (!emailStorage_) {
        LOG_ERROR << "Email storage is not initialized.";
        return;
    }

    std::future<void> executionTask = std::async(std::launch::async, [this]() {
        LOG_INFO << "Executing root plugin asynchronously.";
        EmailListView rootEmailList = emailStorage_->getFullView();
        bool success = rootPluginExecutor_.second->execute(&rootEmailList);
        rootEmailList.commitInserts();
        if (success) {
            LOG_INFO << "Root plugin execution completed successfully. ";
        } else {
            LOG_ERROR << "Root plugin execution failed.";
        }
    });
}

bool GlobalConfigManager::setActiveWorkflowFile(const std::string& workflowFilename) {
    std::lock_guard<std::mutex> lock(mtx_);
    std::ifstream file(WORKFLOWS_DIR + "/" + workflowFilename);

    if (!file.is_open()) {
        return false;
    }
    try {
        file >> CurrentWorkflow_;
        CurrentWorkflowFilename_ = workflowFilename; // Set active workflow
        clear_root_plugin_instance();
        rootPluginExecutor_.second->setConfig(CurrentWorkflow_);
    } catch (const std::exception& e) {
        return false;
    }
    return true;
}

std::string GlobalConfigManager::getActiveWorkflowFileName() const {
    return CurrentWorkflowFilename_;
}

bool GlobalConfigManager::createNewWorkflowFile(const std::string& workflowFilename) {
    std::lock_guard<std::mutex> lock(mtx_);
    std::ofstream file(WORKFLOWS_DIR + "/" + workflowFilename);

    if (!file.is_open()) {
        return false;
    }
    try {
        file << CurrentWorkflow_;
        CurrentWorkflowFilename_ = workflowFilename;
        clear_root_plugin_instance();
        rootPluginExecutor_.second->setConfig(CurrentWorkflow_);
    } catch (const std::exception& e) {
        return false;
    }
    return true;
}