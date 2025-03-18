#include "ArgumentParser.hpp"
#include "GlobalConfigManager.hpp"
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <functional>

void ArgumentParser::parse(int argc, char* argv[]) {
    bool configSupplied = false;

    std::vector<        // Define a vector of
        std::pair<             // pairs where each pair has:
            std::vector<std::string>, // a vector of option strings (aliases)
            std::function<void(int&, int, char*[])>>> // and a lambda that processes that option.
    handlers = {
        { {"-v", "--verbose"},
            [](int &i, int argc, char* argv[])
            {
                Logger::getInstance()->setVerbosity(Logger::Verbosity::HIGH);
                Logger::getInstance()->setMinimumLogLevel(Logger::LogLevel::INFO);
            }
        },
        { {"-vv", "--vverbose"},
            [](int &i, int argc, char* argv[])
            {
                Logger::getInstance()->setVerbosity(Logger::Verbosity::HIGH);
                Logger::getInstance()->setMinimumLogLevel(Logger::LogLevel::DEBUG_VERBOSE);
            }
        },
        { {"-c", "--config"},
            [&configSupplied](int &i, int argc, char* argv[])
            {
                if (i + 1 < argc) {
                    GlobalConfigManager::getInstance()->setConfig(argv[++i]);
                    configSupplied = true;
                }
            }
        },
        { {"-h", "--help"},
            [](int &i, int argc, char* argv[])
            {
               getInstance()->printHelp();
               exit(0);
            }
        },
        { {"-i", "--init"},
            [](int &i, int argc, char* argv[])
            {
               getInstance()->generateNewGlobalConfig();
               exit(0);
            }
        }
    };

    // Iterate over the arguments starting at index 1 (skip program name).
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        bool handled = false;
        // Iterate over our handlers
        for (auto &handler : handlers) {
            const auto &opts = handler.first;
            const auto &func = handler.second;
            if (std::find(opts.begin(), opts.end(), arg) != opts.end()) {
                func(i, argc, argv);  // The lambda may consume extra arguments by modifying i.
                handled = true;
                break;
            }
        }
        if (!handled) {
            LOG_ERROR << "Error: Unknown option " << arg;
            printHelp();
            exit(0);
        }
    }
    if (!configSupplied) {
        LOG_ERROR << "No config file supplied. Generating new one at ./GlobalConfig.json";
        generateNewGlobalConfig();
        GlobalConfigManager::getInstance()->setConfig("GlobalConfig.json");
    }
    if (!GlobalConfigManager::getInstance()->loadConfig()) {
        LOG_ERROR << "Error loading config from file.";
        Logger::getInstance()->startLogging();
        exit(0);
    }
}

void ArgumentParser::printHelp() {
    LOG_INFO << "\nUsage: inlook_cpp [options]\n"
             << "Options:\n"
             << "  -v, --verbose        First level of verbosity.\n"
             << "  -vv, --vverbose      Second level of verbosity.\n"
             << "  -c, --config <path>  Specify initial configuration file path.\n"
             << "  -h, --help           Show this help message.\n"
             << "  -i, --init           Create an initial empty GlobalConfig.json file.\n";
    Logger::getInstance()->startLogging();
}

void ArgumentParser::generateNewGlobalConfig() {
    std::ofstream MyFile("GlobalConfig.json");
    if (!MyFile.is_open()) {
        LOG_ERROR << "Unable to create new GlobalConfig.json file.";
        return;
    }
    nlohmann::json dummyConfig = R"(
{
    "log_dir" : "logs",
    "hostname": "127.0.0.1",
    "port": 8080
}
    )"_json;
    MyFile << dummyConfig.dump(4);
    MyFile.close();
    LOG_INFO << "New config created as GlobalConfig.json";
}