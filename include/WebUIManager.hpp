#pragma once
#include "crow.h"
#include <unordered_set>
#include <mutex>
#include "Logger.hpp"
#include "PluginExecutorInterface.hpp"
#include "RootPluginExecutor.hpp"

class WebUIManager {
public:
    static WebUIManager* getInstance();
    void startServer();
    void stop();
    ~WebUIManager();



private:
    WebUIManager() = default;
    void runServer();

    // WebSocket Management
    void setupWebSocket();

    // API Routes
    void setupApiRoutes();

    // Sets connection:close and other headers
    static crow::response failure(int code, const std::string &message); // provide http response code
    static crow::response failure(int code);
    static crow::response success(const std::string &message); // always 200
    static crow::response success();

    crow::response getIndex();

    crow::response getStyles(const std::string &path);

    crow::response getJS(const std::string &path);

    crow::response getTabs(const std::string &path);

    crow::response fetchDocsExist();

    crow::response getDocs(const std::string &path);

    crow::response getReadme();

    crow::response instantiateRoot();
    crow::response executeRoot();
    crow::response resetInstances();
    crow::response setWorkflow(const crow::request& req);
    crow::response shutdown();
    crow::response fetchLogs();
    crow::response fetchInitLogs();
    crow::response fetchEmails(int start, int count);
    crow::response fetchEmailCount();
    crow::response fetchGlobalConfig();
    crow::response fetchPluginInstances();
    crow::response fetchPluginNames();
    crow::response fetchPluginInstanceTree();
    crow::response fetchWorkflows();
    crow::response fetchCurrentWorkflow();
    crow::response fetchPluginRegistryImplements(const std::string& path);
    crow::response heartbeat();

    // Helpers
    void notifyClients(const std::string& message);

    crow::SimpleApp app;
    std::unordered_set<crow::websocket::connection*> users;
    std::mutex mtx;
    CrowToCustomLogger crowLogger;
    std::thread serverThread;
};