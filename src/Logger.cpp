#include "Logger.hpp"

#include <iostream>
#include <sstream>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <vector>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <format>
#include <algorithm>
#include <unistd.h>

#include "GlobalConfigManager.hpp"
#include "nlohmann/json.hpp"

#ifdef _WIN32
    #include <windows.h>
    #include <dbghelp.h>
    #pragma comment(lib, "dbghelp.lib")
#else
    #include <execinfo.h>
#endif

#ifdef _WIN32
inline std::vector<std::string> captureStackTrace() {
    std::vector<std::string> stackTrace;
    void* stack[100];
    unsigned short frames = CaptureStackBackTrace(0, 100, stack, nullptr);
    SYMBOL_INFO* symbol = (SYMBOL_INFO*)calloc(sizeof(SYMBOL_INFO) + 256 * sizeof(char), 1);
    symbol->MaxNameLen = 255;
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

    HANDLE process = GetCurrentProcess();
    SymInitialize(process, nullptr, TRUE);

    for (unsigned short i = 0; i < frames; i++) {
        SymFromAddr(process, (DWORD64)(stack[i]), 0, symbol);
        std::ostringstream oss;
        oss << "[" << i << "] " << symbol->Name << " - " << (void*)symbol->Address;
        stackTrace.push_back(oss.str());
    }
    free(symbol);
    return stackTrace;
}
#else
inline std::vector<std::string> captureStackTrace() {
    std::vector<std::string> stackTrace;
    void* array[50];
    size_t size = backtrace(array, 50);
    char** symbols = backtrace_symbols(array, size);

    if (symbols) {
        for (size_t i = 0; i < size; i++) {
            stackTrace.push_back(symbols[i]);
        }
        free(symbols);
    }
    return stackTrace;
}
#endif

// ========================= LogEntry Member Functions =========================

nlohmann::json Logger::LogEntry::toJson() const {
    nlohmann::json jsonEntry = {
        {"timestamp", timestamp},
        {"level", Logger::logLevelToString(level)},
        {"file", file},
        {"line", line},
        {"message", message}
    };
    if (!stackTrace.empty() && !(stackTrace.size() == 1 && stackTrace[0].empty())) {
        jsonEntry["stack_trace"] = stackTrace;
    }
    return jsonEntry;
}

std::string Logger::LogEntry::toString(bool verbose) const {
    std::ostringstream oss;
    oss << timestamp << " [" << Logger::logLevelToString(level) << "] " << message;
    if (verbose) {
        oss << " (" << file << ":" << line << ")";
        if (level == ERROR) {
            if (!stackTrace.empty()) {
                oss << "\n========= BEGIN STACK TRACE =========\n";
                for (const auto& frame : stackTrace) {
                    oss << frame << "\n";
                }
                oss << "========== END STACK TRACE ==========";
            } else {
                oss << "\n=== NO STACK TRACE AVAILABLE ===";
            }
        }
    }
    oss << "\n";
    return oss.str();
}

// ========================= Logger Public Functions =========================

std::string Logger::logLevelToString(LogLevel level) {
    switch (level) {
        case INFO:          return "         INFO";
        case WARNING:       return "      WARNING";
        case ERROR:         return "        ERROR";
        case DEBUG_VERBOSE: return "DEBUG_VERBOSE";
        default:            return "UNKNOWN";
    }
}

Logger* Logger::getInstance() {
    static Logger instance;
    return &instance;
}

void Logger::setVerbosity(Verbosity level) {
    verbosityLevel_ = level;
}

void Logger::startLogging() {
    if (!stopFlag_) return;

    stopFlag_ = false;
    try {
        log_dir_ = GlobalConfigManager::getInstance()->getGlobalConfigValue<std::string>("log_dir", "logs");
        std::filesystem::create_directory(log_dir_);
        auto now = std::chrono::system_clock::now();
        logFileName_ = std::format("{}/log-{:%Y-%m-%dT%H-%M-%S}.jsonl", log_dir_, now);
        flushThread_ = new std::thread(&Logger::flushLoop, this);
    } catch (const std::exception &e) {
        std::cerr << e.what();
        stopFlag_ = true;
    }
}

Logger::Verbosity Logger::getVerbosity() const {
    return verbosityLevel_;
}

void Logger::setMinimumLogLevel(LogLevel level) {
    minimumLogLevel_ = level;
}

Logger::LogLevel Logger::getMinimumLogLevel() const {
    return minimumLogLevel_;
}

void Logger::log(LogLevel level, const std::string& message, const char* file, int line) {
    if (level >= minimumLogLevel_) {
        auto now = std::chrono::system_clock::now();
        auto formatted_time = std::format("{:%Y-%m-%dT%H:%M:%SZ}", now);
        LogEntry logEntry = {
            formatted_time,
            level,
            file ? file : "N/A",
            (line != 0 ? line : -1),
            message,
            (level == ERROR ? captureStackTrace() : std::vector<std::string>{""})
        };
        {
            std::lock_guard<std::mutex> lock(*mtx_);
            logBuffer_.push_back(logEntry);
        }
        cv_->notify_one();
    }
}

size_t Logger::estimateTotalSize(const std::vector<std::string>& messages) {
    size_t totalSize = 0;
    for (const auto& msg : messages) {
        totalSize += msg.size();
    }
    return totalSize;
}

void Logger::printLogEntry(const LogEntry& logEntry) {
    std::cout << logEntry.toString(verbosityLevel_ == HIGH);
}

std::string Logger::getWebLogs() {
    std::string returnable;
    {
        std::lock_guard<std::mutex> lock(*mtx_);
        returnable = webLogs_.dump();
        webLogs_.clear();
    }
    return returnable;
}

std::string Logger::getAllWebLogs() {
    std::ifstream file(logFileName_, std::ios::in | std::ios::binary);
    if (!file) {
        LOG_ERROR << "Unable to open log file: " << logFileName_;
        return "[]";
    }
    nlohmann::json logsArray = nlohmann::json::array();
    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty()) {
            try {
                logsArray.push_back(nlohmann::json::parse(line));
            } catch (const std::exception& e) {
                LOG_ERROR << "Error parsing log line: " << e.what();
            }
        }
    }
    // Clear webLogs_ so that previously sent logs are not re-sent.
    {
        std::lock_guard<std::mutex> lock(*mtx_);
        webLogs_.clear();
    }
    return logsArray.dump();
}

void Logger::flush(std::ofstream &logFile) {
    std::vector<LogEntry> logsToFlush;
    {
        std::unique_lock<std::mutex> lock(*mtx_);
        cv_->wait_for(lock, std::chrono::milliseconds(100), [this] { return stopFlag_ || !logBuffer_.empty(); });
        if (!logBuffer_.empty()) {
            logsToFlush.swap(logBuffer_);
        }
    }
    if (!logsToFlush.empty()) {
        for (const auto& log : logsToFlush) {
            printLogEntry(log);
            if (!stopFlag_) {
                logFile << log.toJson().dump() << "\n";
                webLogs_.push_back(log.toJson());
            }
        }
        logFile.flush();
        std::cout.flush();
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

void Logger::flushLoop() {
    std::ofstream logFile(logFileName_, std::ios::app);
    if (!logFile) {
        LOG_ERROR << "Failed to open log file: " << logFileName_;
    }
    while (!logBuffer_.empty()) {
        flush(logFile);
    }
    logFile.close();
}

Logger::~Logger() {
    stopFlag_ = true;
    sleep(1); // Let other classes write, and flushLoop() quit, then call flush one more time.
    {
        std::lock_guard<std::mutex> lock(*mtx_);
        cv_->notify_all();
    }
    if (flushThread_ && flushThread_->joinable()) {
        flushThread_->join();
    }
    delete flushThread_;
    delete mtx_;
    delete cv_;
}

LogStream::LogStream(Logger::LogLevel level, const char* file, int line)
    : level_(level), file_(file), line_(line) {}

LogStream::LogStream(const std::string& levelStr, const char* file, int line)
    : level_(Logger::DEBUG_VERBOSE), file_(file), line_(line) {
}

LogStream::~LogStream() {
    Logger::getInstance()->log(level_, stream_.str(), file_, line_);
}

void CrowToCustomLogger::log(std::string message, crow::LogLevel level) {
    Logger::LogLevel mappedLevel;
    std::string writeMessage;
    switch (level) {
        case crow::LogLevel::DEBUG:
            writeMessage = "(crow::DEBUG))";
            mappedLevel = Logger::DEBUG_VERBOSE;
        break;
        case crow::LogLevel::INFO:
            writeMessage = "(crow::INFO)";
            mappedLevel = Logger::DEBUG_VERBOSE;
        break;
        case crow::LogLevel::WARNING:
            writeMessage = "(crow::WARNING)";
            mappedLevel = Logger::DEBUG_VERBOSE;
        break;
        case crow::LogLevel::ERROR:
            writeMessage = "(crow::ERROR)";
            mappedLevel = Logger::ERROR;
        break;
        default:
            writeMessage = "(crow::Unknown)";
            mappedLevel = Logger::DEBUG_VERBOSE;
        break;
    }
    LogStream(mappedLevel, __FILE__, __LINE__) << writeMessage << " " << message;
}
// ========================= Initialization of Static Members =========================

Logger::Logger() :
    stopFlag_(true),
    log_dir_("logs"),
    verbosityLevel_(LOW),
    minimumLogLevel_(INFO){}