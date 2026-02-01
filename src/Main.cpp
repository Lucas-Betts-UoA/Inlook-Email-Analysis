#include <iostream>

#include "Logger.hpp"
#include "PluginInterface.hpp"
#include "ArgumentParser.hpp"
#include "EmailListView.hpp"
#include "GlobalConfigManager.hpp"
#include "PluginRegistry.hpp"
#include "WebUIManager.hpp"


int main(int argc, char* argv[]) {
    LOG_INFO << "Program started.";
    ArgumentParser::getInstance()->parse(argc, argv);
    Plugins->loadAllPlugins(); // Loads the CreateFunc and handle for all plugins as needed.
    GlobalConfigManager::getInstance()->initialize_root_plugin_instance(); // Initializes the root instance.
    Logger::getInstance()->startLogging(); // Logging on
    WebUIManager::getInstance()->startServer(); // Hang the main thread with the WebUI.
}