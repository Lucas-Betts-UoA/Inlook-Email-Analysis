#pragma once

#include <condition_variable>
#include <sstream>
#include <string>
#include <vector>
#include <thread>
#include <crow/logging.h>

#include "nlohmann/json.hpp"


/// Maximum number of log entries for internal buffering.
#define MAX_LOG_ENTRIES 200

/**
 * @brief Singleton Logger class that handles thread-safe logging.
 *
 * Logger supports multiple log levels and verbosity settings, writes logs to console and file,
 * and maintains log entries for the web interface.
 */
class Logger {
public:
    /// Enumeration of log levels.
    enum LogLevel {
        DEBUG_VERBOSE = 0, ///< Detailed debug information.
        INFO = 1,        ///< General informational messages.
        WARNING = 2,     ///< Warning messages.
        ERROR = 3        ///< Error messages.
    };

    /// Enumeration of verbosity levels.
    enum Verbosity {
        LOW,  ///< Only log message.
        HIGH  ///< Log message with file and line information.
    };

    /**
     * @brief Represents a single log entry.
     */
    struct LogEntry {
        std::string timestamp;
        LogLevel level;
        std::string file;
        int line;
        std::string message;
        std::vector<std::string> stackTrace;

        /**
         * @brief Converts the log entry to a JSON object.
         * @return A nlohmann::json object representing the log entry.
         */
         nlohmann::json toJson() const;

        /**
         * @brief Converts the log entry to a human-readable string.
         * @param verbose If true, include file, line, and stack trace information.
         * @return The formatted log entry as a string.
         */
         std::string toString(bool verbose) const;
    };

    /**
     * @brief Converts a LogLevel enum to a string.
     * @param level The log level.
     * @return A string representation of the log level.
     */
    static std::string logLevelToString(LogLevel level);

    /**
     * @brief Gets the singleton Logger instance.
     * @return A pointer to the Logger instance.
     */
    static Logger* getInstance();

    /**
     * @brief Sets the verbosity level.
     * @param level The desired verbosity level.
     */
    void setVerbosity(Verbosity level);

    /**
     * @brief Starts the logging system.
     */
    void startLogging();

    /**
     * @brief Gets the current verbosity level.
     * @return The current verbosity level.
     */
     Verbosity getVerbosity() const;

    /**
     * @brief Sets the minimum log level for messages to be logged.
     * @param level The minimum log level.
     */
    void setMinimumLogLevel(LogLevel level);

    /**
     * @brief Gets the current minimum log level.
     * @return The current minimum log level.
     */
     LogLevel getMinimumLogLevel() const;

    /**
     * @brief Logs a message.
     *
     * Formats the message with a timestamp and (if in high verbosity mode) file and line information,
     * then adds the log entry to the internal buffer and notifies the flush thread.
     *
     * @param level The log level.
     * @param message The log message.
     * @param file The source file (optional).
     * @param line The source line (optional).
     */
    void log(LogLevel level, const std::string &message, const char *file = nullptr, int line = 0);

    /**
     * @brief Estimates the total size (in bytes) of a vector of strings.
     * @param messages The vector of strings.
     * @return The total size.
     */
     static size_t estimateTotalSize(const std::vector<std::string>& messages) ;

    /**
     * @brief Prints a log entry to the console.
     * @param logEntry The log entry to print.
     */
    void printLogEntry(const LogEntry &logEntry);

    /**
     * @brief Retrieves the buffered logs for the web interface and clears the web log buffer.
     * @return A JSON string representing the web logs.
     */
    std::string getWebLogs();

    /**
     * @brief Loads the entire log file from disk and returns it as a JSON array string.
     * @return A JSON array string containing all log entries.
     */
    std::string getAllWebLogs();

    ~Logger();

private:
    Logger();
    Logger(const Logger &) = delete;
    Logger &operator=(const Logger &) = delete;

    // Internal state and synchronization:
    std::mutex*mtx_ = new std::mutex();                     // Pointer to mutex for thread safety
    std::condition_variable *cv_ = new std::condition_variable();         // Pointer to condition variable for signaling
    std::thread* flushThread_{};            // Pointer to the flush thread
    bool stopFlag_;                       // Flag to indicate shutdown
    std::vector<LogEntry> logBuffer_;     // Buffer for pending log entries
    nlohmann::json webLogs_;              // Buffer for logs to send to the web interface
    std::string log_dir_;                 // Log directory
    std::string logFileName_;             // Current log file name
    Verbosity verbosityLevel_;            // Current verbosity level
    LogLevel minimumLogLevel_;            // Current minimum log level

    /**
     * @brief Internal flush loop that writes log entries from the buffer to disk and web log storage.
     */
    void flushLoop();
    void flush(std::ofstream &logFile);
};

/**
 * @brief Helper class for streaming log messages.
 *
 * LogStream allows you to build log messages using the << operator. When the LogStream is destroyed,
 * it sends the accumulated message to the Logger.
 */
class LogStream {
public:
    /**
     * @brief Constructs a LogStream with a given log level, file, and line number.
     * @param level The log level.
     * @param file The source file where the log is generated.
     * @param line The line number in the source file.
     */
    LogStream(Logger::LogLevel level, const char *file, int line);

    /**
     * @brief Constructs a LogStream with a given log level (as string), file, and line number.
     * @param levelStr The log level as a string.
     * @param file The source file.
     * @param line The source line.
     */
    LogStream(const std::string& levelStr, const char *file, int line);

    /**
     * @brief Destructor. Sends the accumulated log message to the Logger.
     */
    ~LogStream();

    /**
     * @brief Overloaded << operator to stream data into the log message.
     *
     * @tparam T The type of data.
     * @param data The data to append to the log message.
     * @return Reference to the current LogStream.
     */
    template<typename T>
    LogStream &operator<<(const T &data) {
        stream_ << data;
        return *this;
    }

private:
    Logger::LogLevel level_;   ///< The severity level.
    const char* file_;         ///< Source file.
    int line_;                 ///< Source line.
    std::ostringstream stream_;///< Stream accumulating the log message.
};

// Logging macros for convenience.
#define LOG_DEBUG_VERBOSE   LogStream(Logger::DEBUG_VERBOSE, __FILE__, __LINE__)
#define LOG_INFO            LogStream(Logger::INFO, __FILE__, __LINE__)
#define LOG_WARNING         LogStream(Logger::WARNING, __FILE__, __LINE__)
#define LOG_ERROR           LogStream(Logger::ERROR, __FILE__, __LINE__)

// Forward declaration of CrowToCustomLogger if needed.
// Custom log interface to handle Crow's logs.
class CrowToCustomLogger final : public crow::ILogHandler {
public:
    void log(std::string message, crow::LogLevel level) override;
};