#include "WebUIManager.hpp"
#include "EmailListView.hpp"
#include "EmailStorage.hpp"
#include "GlobalConfigManager.hpp"
#include "PluginRegistry.hpp"
#include <fstream>
#include <sstream>

std::optional<std::string> sanitizePath(const std::string& requestedPath) {
    const std::string baseDir = "web"; // No calls should be above this scope.
    try {
        std::filesystem::path fullPath = std::filesystem::weakly_canonical(std::filesystem::path(baseDir) / requestedPath);
        std::filesystem::path allowedBase = std::filesystem::canonical(baseDir);

        // Ensure the requested file is within the base directory
        if (fullPath.string().find(allowedBase.string()) != 0) {
            LOG_ERROR << "Blocked directory traversal attempt: " << requestedPath;
            return std::nullopt;
        }

        return requestedPath;
    } catch (const std::exception& e) {
        LOG_ERROR << "Path sanitization failed for: " << requestedPath << " - " << e.what();
        return std::nullopt;
    }
}

WebUIManager::~WebUIManager() {
    LOG_DEBUG_VERBOSE << "WebUIManager::~WebUIManager()";
    stop();
}

WebUIManager* WebUIManager::getInstance() {
    static WebUIManager instance;
    return &instance;
}

void WebUIManager::startServer() {
    LOG_DEBUG_VERBOSE << "WebUIManager::startServer()";
    if (GlobalConfigManager::getInstance()->isReady()) {
        crow::logger::setHandler(&crowLogger);
        crow::logger::setLogLevel(crow::LogLevel::INFO);
        crow::mustache::set_global_base("web/components/");
        serverThread = std::thread(&WebUIManager::runServer, this);
        serverThread.join();
    } else {
        LOG_ERROR << "GlobalConfigManager did not have root instance ready, aborting WebUIManager::startServer()";
    }
}

void WebUIManager::stop() {
    LOG_DEBUG_VERBOSE << "WebUIManager::stop()";
    if (serverThread.joinable()) {
        serverThread.join();
    }
}

void WebUIManager::runServer() {
    setupWebSocket();
    setupApiRoutes();
    LOG_INFO << "We have set up routes!";

    app.bindaddr(GlobalConfigManager::getInstance()->getGlobalConfigValue<std::string>("hostname"))
       .port(GlobalConfigManager::getInstance()->getGlobalConfigValue<uint16_t>("port"))
       .loglevel(crow::LogLevel::Warning)
       .multithreaded()
       .run();
}

void WebUIManager::setupWebSocket() {
    CROW_WEBSOCKET_ROUTE(app, "/ws")
      .onopen([&](crow::websocket::connection& conn) {
          CROW_LOG_INFO << "New WebSocket connection from " << conn.get_remote_ip();
          std::lock_guard<std::mutex> lock(mtx);
          users.insert(&conn);
      })
      .onclose([&](crow::websocket::connection& conn, const std::string& reason, uint16_t) {
          CROW_LOG_INFO << "WebSocket closed: " << reason;
          std::lock_guard<std::mutex> lock(mtx);
          users.erase(&conn);
      });
}

void WebUIManager::setupApiRoutes() {
    CROW_ROUTE(app, "/")
        ([this] {return getIndex();});

    CROW_ROUTE(app, "/style/<path>")
        ([this](const std::string& path) {return getStyles(path);});

    CROW_ROUTE(app, "/js/<path>")
        ([this](const std::string& path) {return getJS(path);});

    CROW_ROUTE(app, "/tabs/<path>")
        ([this](const std::string& path) {return getTabs(path);});

    CROW_ROUTE(app, "/docs/<path>")
        ([this](const std::string& path) {return getDocs(path);});

    CROW_ROUTE(app, "/api/v1/fetch-docs-exist")
        ( [this] { return fetchDocsExist();});

    CROW_ROUTE(app, "/api/v1/fetch-init-logs")
        ([this] {return fetchInitLogs();});

    CROW_ROUTE(app, "/api/v1/readme")
        ([this]() { return getReadme();});

    CROW_ROUTE(app, "/api/v1/instantiate-root-recursive").methods(crow::HTTPMethod::POST)
        ([this] { return instantiateRoot(); });

    CROW_ROUTE(app, "/api/v1/execute-root-recursive").methods(crow::HTTPMethod::POST)
        ([this] { return executeRoot(); });

    CROW_ROUTE(app, "/api/v1/reset-all-instances").methods(crow::HTTPMethod::POST)
        ([this] { return resetInstances(); });

    CROW_ROUTE(app, "/api/v1/set-workflow").methods(crow::HTTPMethod::POST)
        ([this](const crow::request& req) { return setWorkflow(req); });

    CROW_ROUTE(app, "/api/v1/shutdown").methods(crow::HTTPMethod::POST)
        ([this] { return shutdown(); });

    CROW_ROUTE(app, "/api/v1/fetch-logs")
        ([this] { return fetchLogs(); });

    CROW_ROUTE(app, "/api/v1/fetch-email-count")
        ([this] { return fetchEmailCount(); });

    CROW_ROUTE(app, "/api/v1/fetch-emails/<int>/<int>")
        ([this](int start, int count) { return fetchEmails(start, count); });

    CROW_ROUTE(app, "/api/v1/fetch-global-config")
        ([this] { return fetchGlobalConfig(); });

    CROW_ROUTE(app, "/api/v1/fetch-plugin-instanceIDs")
        ([this] { return fetchPluginInstances(); });

    CROW_ROUTE(app, "/api/v1/fetch-plugin-name")
        ([this] { return fetchPluginNames(); });

    CROW_ROUTE(app, "/api/v1/fetch-plugin-instance-tree")
        ([this] { return fetchPluginInstanceTree(); });

    CROW_ROUTE(app, "/api/v1/fetch-workflows")
        ([this] { return fetchWorkflows(); });

    CROW_ROUTE(app, "/api/v1/fetch-current-workflow")
        ([this] { return fetchCurrentWorkflow(); });

    CROW_ROUTE(app, "/api/v1/fetch-plugin-registry-implements/<path>")
        ([this](const std::string& path) { return fetchPluginRegistryImplements(path); });

    CROW_ROUTE(app, "/api/v1/heartbeat")
        ([this] { return heartbeat(); });
}

// Helper functions for responses
crow::response WebUIManager::failure(int code, const std::string& message) {
    crow::response res(code, message);
    res.set_header("Connection", "close");
    return res;
}

crow::response WebUIManager::failure(int code) {
    return failure(code, "Failure");
}


crow::response WebUIManager::success(const std::string& message) {
    crow::response res(200, message);
    res.set_header("Connection", "close");
    return res;
}

crow::response WebUIManager::success() {
    return success("Success");
}

crow::response WebUIManager::getIndex() {
    LOG_DEBUG_VERBOSE << "WebUIManager::getIndex()";
    std::ifstream file("web/index.html", std::ios::binary);
    if (!file) return crow::response(404);
    std::ostringstream content;
    content << file.rdbuf();
    return success(content.str());
}

crow::response WebUIManager::getStyles(const std::string& path) {
    LOG_DEBUG_VERBOSE << "WebUIManager::getStyles("<<path<<")";
    auto safePath = sanitizePath(path);
    if (!safePath) return crow::response(403, "Forbidden");
    std::ifstream file("web/style/" + safePath.value(), std::ios::binary);
    if (!file) return crow::response(404);
    std::ostringstream content;
    content << file.rdbuf();
    return success(content.str());
}

crow::response WebUIManager::getJS(const std::string& path) {
    LOG_DEBUG_VERBOSE << "WebUIManager::getJS("<<path<<")";
    auto safePath = sanitizePath(path);
    if (!safePath) return crow::response(403, "Forbidden");
    std::ifstream file("web/js/" + safePath.value(), std::ios::binary);
    if (!file) return failure(404);
    std::ostringstream content;
    content << file.rdbuf();
    return success(content.str());
}

crow::response WebUIManager::getTabs(const std::string& path) {
    LOG_DEBUG_VERBOSE << "WebUIManager::getTabs("<<path<<")";
    auto safePath = sanitizePath(path);
    if (!safePath) return crow::response(403, "Forbidden");
    std::ifstream file("web/tabs/" + safePath.value(), std::ios::binary);
    if (!file) return crow::response(404);
    std::ostringstream content;
    content << file.rdbuf();
    return success(content.str());
}

crow::response WebUIManager::fetchDocsExist() {
    std::string docsDir = "web/docs/html";
    bool exists = std::filesystem::exists(docsDir);
    LOG_DEBUG_VERBOSE << "WebUIManager::fetchDocsExist()";
    return exists ? success() : failure(404);
}

crow::response WebUIManager::getDocs(const std::string& path) {
    LOG_DEBUG_VERBOSE << "WebUIManager::getDocs("<<path<<")";
    auto safePath = sanitizePath(path);
    if (!safePath) return crow::response(403, "Forbidden");
    std::ifstream file("web/docs/html/" + safePath.value(), std::ios::binary);
    if (!file) return crow::response(404);
    std::ostringstream content;
    content << file.rdbuf();
    return success(content.str());
}

crow::response WebUIManager::getReadme() {
    std::filesystem::path readmePath = std::filesystem::current_path() / "README.md";
    if (!std::filesystem::exists(readmePath)) return crow::response(404, "README.md not found");

    std::ifstream file(readmePath);
    if (!file) return crow::response(500, "Failed to open README.md");

    std::ostringstream content;
    content << file.rdbuf();

    return success(content.str());
}

crow::response WebUIManager::instantiateRoot() {
    try {
        GlobalConfigManager::getInstance()->getRootPluginExecutor()->instantiateRecursive();
        return success();
    } catch (const std::exception& e) {
        return failure(500, e.what());
    }
}

crow::response WebUIManager::executeRoot() {
    try {
        GlobalConfigManager::getInstance()->executeRootAsync(); // using the global emailList
        return success();
    } catch (const std::exception& e) {
        return failure(500, e.what());
    }
}

crow::response WebUIManager::resetInstances() {
    try {
        GlobalConfigManager::getInstance()->clear_root_plugin_instance();
        return success();
    } catch (const std::exception& e) {
        return failure(500, e.what());
    }
}

crow::response WebUIManager::setWorkflow(const crow::request &req) {
    try {
        bool status = false;
        const auto body = crow::json::load(req.body);
        if (body && body.has("workflow")) {
            status = GlobalConfigManager::getInstance()->setActiveWorkflowFile(body["workflow"].s());
            if (status) return success();
        }

    } catch (const std::exception& e) {
        return failure(500, e.what());
    }
    return failure(500);
}


crow::response WebUIManager::fetchLogs() {
    try {
        return success(Logger::getInstance()->getWebLogs());
    } catch (const std::exception& e) {
        return failure(500, e.what());
    }
}

crow::response WebUIManager::fetchInitLogs() {
    try {
        auto logs = Logger::getInstance()->getAllWebLogs();
        return success(logs);
    } catch (const std::exception& e) {
        return failure(500, e.what());
    }
}

crow::response WebUIManager::fetchEmails(int start, int count) {
    try {
        auto emails = GlobalConfigManager::getInstance()->getEmailListPointer()->getEmailsByNumber(start, count);
        return success(emails.dump(4, ' ', false, nlohmann::detail::error_handler_t::ignore));
    } catch (const std::exception& e) {
        return failure(500, e.what());
    }
}

crow::response WebUIManager::fetchEmailCount() {
    try {
        int count = GlobalConfigManager::getInstance()->getEmailListPointer()->getSize();
        return success(std::to_string(count));
    } catch (const std::exception& e) {
        return failure(500, e.what());
    }
}

crow::response WebUIManager::fetchGlobalConfig() {
    try {
        return success(GlobalConfigManager::getInstance()->getGlobalConfig().dump());
    } catch (const std::exception& e) {
        return failure(500, e.what());
    }
}

crow::response WebUIManager::fetchPluginInstances() {
    try {
        return success(Plugins->listInstances().dump());
    } catch (const std::exception& e) {
        return failure(500, e.what());
    }
}

crow::response WebUIManager::fetchPluginNames() {
    try {
        nlohmann::json const pluginNames = Plugins->listAvailablePlugins();
        return success(pluginNames.dump());
    } catch (const std::exception& e) {
        return failure(500, e.what());
    }
}

crow::response WebUIManager::fetchPluginInstanceTree() {
    try {
        return success(GlobalConfigManager::getInstance()->getRootPluginExecutor()->printRecursiveInstanceTreeJson().dump());
    } catch (const std::exception& e) {
        return failure(500, e.what());
    }
}

crow::response WebUIManager::fetchWorkflows() {
    try {
        return success(GlobalConfigManager::getInstance()->listWorkflowJsonFiles().dump());
    } catch (const std::exception& e) {
        return failure(500, e.what());
    }
}

crow::response WebUIManager::fetchCurrentWorkflow() {
    try {
        return success(GlobalConfigManager::getInstance()->getCurrentWorkflow().dump());
    } catch (const std::exception& e) {
        return failure(500, e.what());
    }
}

crow::response WebUIManager::fetchPluginRegistryImplements(const std::string& path) {
    try {
        return success(Plugins->listImplementedInterfaces(path).dump());
    } catch (const std::exception& e) {
        return failure(500, e.what());
    }
}

crow::response WebUIManager::heartbeat() {
    return success();
}


void WebUIManager::notifyClients(const std::string& message) {
    std::lock_guard<std::mutex> lock(mtx);
    for (auto* conn : users) {
        conn->send_text(message);
    }
}

crow::response WebUIManager::shutdown() {
    app.stop();
    return success("Server shutting down...");
}
