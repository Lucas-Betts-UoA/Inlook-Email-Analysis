import os
import sys


def create_project_structure(classname):
    # Define the folder structure
    root_folder = f"{classname}"
    subfolders = [f"src", f"include"]
    files = [
        (
            os.path.join(root_folder,
                         subfolders[0],
                         f"{classname}.cpp"),
f"""
#include "{classname}.hpp"

#include "Logger.hpp"
#include "EmailList.hpp"
#include <PluginInterface.hpp>
#include <unordered_map>
#include <iostream>
#include <functional>
#include <string>
#include <regex>
#include "Email.hpp"
#include <vector>
#include <filesystem>

// Constructor
{classname}::{classname}(const std::string& instanceID) : PluginRunnableInterface(instanceID) {{
    pluginName_ = "{classname}";
        instanceID_ = instanceID;
        optionSchema_ = R"(
        {{
        "$schema": "https://json-schema.org/draft/2020-12/schema",
        "type": "object",
        "properties": {{
        }},
        "required": [
        ],
        "additionalProperties": false
        }}
        )"_json;
        inputAttributes_ = {{}};
    generatedAttributes_ = {{}};
    SET_PLUGIN_STATE("LOADED");
}}

bool {classname}::instantiateRecursive() {{
    SET_PLUGIN_STATE("READY");
    return true;
}}

nlohmann::json {classname}::printRecursiveInstanceTreeJson() {{
nlohmann::json node;
node["instanceID"] = instanceID_;
node["createFunc"] = Plugins->getCreateFuncForInstance(instanceID_);
node["state"]     = getState();
return node;
}}

// Destructor
{classname}::~{classname}() = default;

int {classname}::execute(EmailList *emailList) {{
LOG_INFO << "{classname}::execute called.";
SET_PLUGIN_STATE("RUNNING");
for (auto email : *emailList) {{
    LOG_DEBUG_VERBOSE << "Email Parsed by {classname}";
}}
SET_PLUGIN_STATE("COMPLETE");
return true;
}}
"""),
        (
            os.path.join(root_folder,
                         subfolders[1],
                         f"{classname}.hpp"),
f"""
#pragma once
#include "PluginRunnableInterface.hpp"


// {classname} class implementing PluginInterface
class  final : public PluginRunnableInterface {{
public:
    explicit {classname}(const std::string&);  // Constructor
    ~{classname}() override;  // Destructor

    bool instantiateRecursive() override;
    nlohmann::json printRecursiveInstanceTreeJson() override;

    int execute(EmailList *emailList) override;

private:
    struct Register {{
        Register() {{
            std::string name = "{classname}";
            LOG_DEBUG_VERBOSE << "Registering plugin " << name;
            Plugins->registerPlugin(name, [](const std::string& instanceID) -> PluginInterface* {{
                return new {classname}(instanceID);
            }});
        }}
    }};
    static inline Register reg;
}};
"""),
        (
            os.path.join(root_folder,
                         f"CMakeLists.txt"),
f"""
project({classname} LANGUAGES CXX)

# Enable verbose output during the build process (optional)
set(CMAKE_VERBOSE_MAKEFILE ON)

# Add the plugin as a shared library
file(GLOB SRC_FILES "${{CMAKE_CURRENT_LIST_DIR}}/src/*.cpp")
include_directories(include)
add_library({classname} SHARED ${{SRC_FILES}})

# Ensure position-independent code (best practice for shared libraries)
set_target_properties({classname} PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Set the output directory for the plugin
set_target_properties({classname} PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${{CMAKE_BINARY_DIR}}/plugins/${{PROJECT_NAME}}
)
message(STATUS "LIBRARY_OUTPUT_DIRECTORY ${{CMAKE_BINARY_DIR}}/plugins/${{PROJECT_NAME}}/")
"""
        )
    ]

    # Check if the root folder already exists
    if os.path.exists(root_folder):
        print(f"Error: The folder '{root_folder}' already exists.")
        return

    # Create folders
    os.makedirs(root_folder)
    for subfolder in subfolders:
        os.makedirs(os.path.join(root_folder, subfolder))

    # Create files
    for filepath, content in files:
        with open(filepath, 'w') as file:
            file.write(content)

    print(f"Project structure for '{classname}' has been created.")


if __name__ == "__main__":
    # Ensure a classname is provided as a command-line argument
    if len(sys.argv) != 2:
        print("Usage: provide classname for new plugin")
        sys.exit(1)

    classname = sys.argv[1]
    create_project_structure(classname)
