#pragma once
#include <string>
#include <vector>
#include <iostream>
#include <nlohmann/json.hpp>
#include <fstream>
#include "Logger.hpp"

/**
 * @brief Singleton for parsing command-line arguments and updating global configuration.
 *
 * This class processes command-line arguments and directly updates global components
 * such as the GlobalConfigManager and Logger. It no longer returns a JSON configuration,
 * but instead loads it into the global manager.
 */
class ArgumentParser {
public:
    /**
     * @brief Get the singleton instance of ArgumentParser.
     * @return Pointer to the singleton instance.
     */
    static ArgumentParser *getInstance() {
        static ArgumentParser instance;
        return &instance;
    }

    /**
     * @brief Parses command-line arguments.
     *
     * Processes arguments for verbosity, configuration file path, help, and initialization.
     * When a configuration file is specified, it loads the JSON config into the GlobalConfigManager.
     *
     * @param argc Argument count.
     * @param argv Argument vector.
     */
    void parse(int argc, char* argv[]);

    /**
     * @brief Prints help information for command-line usage.
     */
    void printHelp();

    /**
     * @brief Generates a dummy configuration file.
     */
    void generateNewGlobalConfig();

private:
    ArgumentParser() = default;
    ArgumentParser(const ArgumentParser &) = delete;
    ArgumentParser &operator=(const ArgumentParser &) = delete;
};