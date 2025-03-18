# Plugin Development Guide

## Overview
Inlook uses a **plugin-based architecture** to allow users to extend its functionality. Plugins operate on email data and integrate seamlessly into the system using a shared interface.

This document describes the structure of an Inlook plugin, how it interacts with the system, and how to modify an existing plugin or create a new one.

---

## Plugin Structure
Each plugin resides in its own directory under `plugins/`.  
We will use `DummyPlugin` as a reference.

Plugins must loosely follow this structure for building to work:

```plaintext
plugins/
├── DummyPlugin/
├── include/
│   ├── DummyPlugin.hpp  # Plugin class header
├── src/
│   ├── DummyPlugin.cpp  # Plugin implementation
├── CMakeLists.txt       # Plugin build configuration
```

### **Creating a New Plugin**
Use the plugin generator script:
```sh
cd plugins
python3 newNamedPlugin.py MyNewPlugin
```

This will create:
```plaintext
plugins/MyNewPlugin/
├── include/MyNewPlugin.hpp
├── src/MyNewPlugin.cpp
├── CMakeLists.txt
├── schema.json
```

Ensure the generated files match your intended plugin name.

---

## **Understanding Plugin Interfaces**
All plugins in Inlook inherit from one of three core interfaces:

### **1. `PluginInterface`**
This is the **base interface** that all plugins must inherit from.  
It defines the essential lifecycle methods (`instantiate()`, `execute()`, `destroy()`) and basic metadata such as plugin names and configurations.

### **2. `PluginExecutorInterface`**
This extends `PluginInterface` and is used for **non-recursive, one-time execution** of plugins.  
It is suitable for plugins that perform **single-pass operations** (e.g., scanning an email list and producing a report).

If your plugin **does not need to instantiate sub-plugins or manage hierarchical execution**, use `PluginExecutorInterface`.

### **3. `PluginRunnableInterface`**
This extends `PluginInterface` and is used for **hierarchical, recursive execution**.  
It is required for **workflow-based plugins** that instantiate sub-plugins dynamically.  
For example:
- A **workflow controller** plugin that delegates processing to multiple sub-plugins.
- A **multi-stage email processor** that applies multiple filters in sequence.

---

## **Key Components**

### **1. Plugin Header (`include/DummyPlugin.hpp`)**
Defines the plugin class, inheriting from `PluginRunnableInterface`.

```cpp
#pragma once
#include "PluginRunnableInterface.hpp"

// DummyPlugin class implementing PluginInterface
class DummyPlugin final : public PluginRunnableInterface {
public:
    explicit DummyPlugin(const std::string&);  // Constructor
    ~DummyPlugin() override;  // Destructor

    bool instantiateRecursive() override;
    nlohmann::json printRecursiveInstanceTreeJson() override;

    bool execute(EmailListView * emailList) override;

private:
    struct Register {
        Register() {
            std::string name = "DummyPlugin";
            LOG_DEBUG_VERBOSE << "Registering plugin " << name;
            Plugins->registerPlugin(name, [](const std::string& instanceID) -> PluginInterface* {
                return new DummyPlugin(instanceID);
            });
        }
    };
    static inline Register reg;
};
```

#### **Modifications**
- **Class Name:** Modify all instances of `DummyPlugin` to your plugin's name.
- **Registration:** Ensure `struct Register` uses the correct name. This should be automatically updated by the script.
- **Interface Methods:** Modify `execute()`, `instantiateRecursive()`, and `printRecursiveInstanceTreeJson()` based on your plugin’s functionality.

---

### **2. Plugin Implementation (`src/DummyPlugin.cpp`)**
Implements the plugin’s logic.

```cpp
#include "DummyPlugin.hpp"

#include "Logger.hpp"
#include "EmailListView.hpp"
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
DummyPlugin::DummyPlugin(const std::string& instanceID) : PluginRunnableInterface(instanceID) {
    pluginName_ = "DummyPlugin";
    instanceID_ = instanceID;
    optionSchema_ = R"(
{
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "type": "object",
    "properties": {
    },
    "required": [
    ],
    "additionalProperties": false
}
    )"_json;
    inputAttributes_ = {};
    generatedAttributes_ = {};
    SET_PLUGIN_STATE("LOADED");
}

bool DummyPlugin::instantiateRecursive() {
    SET_PLUGIN_STATE("READY");
    return true;
}

nlohmann::json DummyPlugin::printRecursiveInstanceTreeJson() {
    nlohmann::json node;
    try {
        node["instanceID"] = instanceID_;
        node["createFunc"] = Plugins->getCreateFuncForInstance(instanceID_) ? Plugins->getCreateFuncForInstance(instanceID_) : "Not Loaded";
        node["state"]     = getState();
        node["schema"]       = !optionSchema_.empty()? optionSchema_ : "";
        node["config"]       = !optionConfig_.empty() ? optionConfig_ : "";
    } catch (std::exception& e) {
        LOG_ERROR << e.what();
    }
    return node;
}

// Destructor
DummyPlugin::~DummyPlugin() = default;

bool DummyPlugin::execute(EmailListView * emailList) {
    LOG_INFO << "DummyPlugin::execute called.";
    SET_PLUGIN_STATE("RUNNING");
    for (auto email : *emailList) {
        LOG_DEBUG_VERBOSE << "Email " << email.getUniqueHash() << " Parsed by DummyPlugin";
    }
    SET_PLUGIN_STATE("COMPLETE");
    return true;
}
```

#### **Modifications**
- **Constructor:** Add any setup logic for your plugin.
- **Schema (`optionSchema_`)**:
    - Define any configurable parameters for the plugin.
    - Modify the JSON schema to reflect required options.
- **Execution (`execute()`)**:
    - Implement the logic for processing emails.
    - Use `SET_PLUGIN_STATE("RUNNING")` and `SET_PLUGIN_STATE("COMPLETE")` to update the UI.
    - Use logging macros (`LOG_INFO`, `LOG_DEBUG_VERBOSE`) as needed.
- **Interface Methods:** Modify `execute()`, `instantiateRecursive()`, and `printRecursiveInstanceTreeJson()` based on your plugin’s functionality.
---

### **3. CMake Build Configuration (`CMakeLists.txt`)**
Defines how the plugin is compiled and linked.

```cmake
project(DummyPlugin LANGUAGES CXX)

# Enable verbose output during the build process (optional)
set(CMAKE_VERBOSE_MAKEFILE ON)

# Add the plugin as a shared library
file(GLOB SRC_FILES "${CMAKE_CURRENT_LIST_DIR}/src/*.cpp")
include_directories(include)
add_library(DummyPlugin SHARED ${SRC_FILES})

# Ensure position-independent code (best practice for shared libraries)
set_target_properties(DummyPlugin PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Set the output directory for the plugin
set_target_properties(DummyPlugin PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins/${PROJECT_NAME}
)
message(STATUS "LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins/${PROJECT_NAME}/")
```

#### **Modifications**
- Update **`DummyPlugin`** to match your plugin’s name.
- Ensure source files match (`src/*.cpp`).
- Once the plugin folder is created in `plugins/`, CMake will automatically build it.

---

## **Adding Your Plugin to the Workflow**
Once your plugin is created, it must be added to a **workflow JSON** to be used.

1. Open a **workflow file** (`config/workflows/*.json`).
2. Locate the `"plugins"` array.
3. Add an entry for your new plugin (in order of execution), with "options" which match your defined schema:
```json
{
    "...":  "...",
    "plugins": [
        {
            "name": "DummyPlugin",
            "options": {
                "option1": "value",
                "option2": 5
            }
        }
    ]
}
```

---

## **Compiling & Running**
Once your plugin is set up, build as normal.

---

## **Debugging & Testing**
- Use `-v` or `-vv` for **verbose logging** (using LOG_DEBUG_VERBOSE << "What";.
- Ensure schema options match the expected input format.
- Check logs for plugin execution details.
- If errors occur, confirm:
    - Plugin is compiled correctly.
    - Registered in `config.json`.
    - Properly defined input attributes.

---