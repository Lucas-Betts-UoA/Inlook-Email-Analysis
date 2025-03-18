# Inlook - Email Analysis Tool

*last updated: Monday 17th March*

**For research and industry use in analyzing and processing emails efficiently.**

### Contact:
To get in touch, please email:

Lucas Betts
lucas.betts@auckland.ac.nz

This tool has been developed by the University of Auckland Secret Lab.

### Safety Warnings
**Note:** This tool is a work in progress. Expect issues, but feel free to make them known so that we can improve!

**Note:** For research and development purposes only.

**Note:** Analysing malicious emails can be dangerous, and the UI provides a mechanism for these emails to be sent over a network into your browser.

**Note:** If you are concerned, we urge you to run this code in a sandbox.


## Description

Inlook is an extendable, modular tool designed for individuals and organisations to analyze large volumes of digital messages. Initially focused on email analysis, it provides a plugin-based system for creating custom analysis workflows.

The primary goal of Inlook is to serve as a community-driven framework for bulk message analysis, enabling both researchers and industry professionals to develop, share, and deploy custom email analysis workflows. 
Its plugin-based architecture allows email analysis methodology to be easily reproduced using a single workflow (.json) file and shared plugins.

Core Features:
- Flexible Parsing & Processing – Supports standard and MIME multipart email formats in many languages/encodings. However, automated detection of encoding is not always reliable.
- Web-based UI – Provides interactive control, real-time monitoring, and workflow configuration.
- Plugin System – Designed for extension and customization depending on the analysis requirements.
- API-Driven – Provides structured REST and WebSocket interfaces for automation and integration with external systems.

### Sample Workflow + Sample Dataset
We have provided a sample workflow under config/workflows. 

Most of our work uses emails from the [Untroubled.org/spam](https://untroubled.org/spam/) dataset provided by Bruce Guenter. However, the test data directory contains a few spam emails that were personally collected.

### TO-DO:
While this is an open-ended tool with many planned features, the current version has some issues that need addressing:

1. Configurations cannot be edited in the user interface, this process must be done in a "workflow" .json file as we have not yet ensured safety of the filesystem.
2. Global configurations also cannot be edited in the user interface, same as above.
3. Parallel execution has been disabled for safety.
4. Email view in the gui has been reduced to text-only, as we have not yet ensured safety of the js/script that could be contained in an email.
5. The filter with exclude does not currently remove emails from EmailStorage.
6. Most plugins are still being developed, and will be included in the coming weeks.

## Installation & Usage

### Dependencies
For Unix-like systems, ensure the following dependencies are installed:
- **CMake (3.20+)**
- **Build Essentials** (`build-essential`)
- **PostgreSQL Client Libraries** (`libpqxx`)
- **ICU4C** (install via your system's package manager)
- **Python 3** (optional, for plugin management)
- **ASIO Standalone (1.30.2 preferred)** – Download manually and extract into `external/`

### Cloning and Building
```sh
git clone [git-url]
cd inlook
git submodule update --init
mkdir build && cd build
cmake .. && make
```

*Note:* The build process fetches the following required submodules:  
`Crow`, `fastText`, `json`, `json-schema-validator`, `lexbor`, `libpqxx`.  
Ensure **ASIO 1.30.2** is placed in `external/` before building.

### Post-Build Setup
Once built, download **`lid.176.bin`** from:  
[https://fasttext.cc/docs/en/language-identification.html](https://fasttext.cc/docs/en/language-identification.html)  
Place it into the **binary directory** (`build/`).

### Documentation (Optional)
To generate documentation for the UI:
```sh
doxygen
```
This will populate the `docs/` directory, making it available in the web interface.
### Configuring Inlook
#### Generate a Default Configuration File
```sh
./inlook_cpp -i
```
This creates a `globalConfig.json` file which should be edited if you wish to use a non-standard hostname, port or log directory.

#### Running with a Config File
```sh
./inlook_cpp -c dummyConfig.json
```

### Command-Line Options
```sh
Usage: inlook_cpp [options]
Options:
  -v, --verbose        First level of verbosity.
  -vv, --vverbose      Second level of verbosity.
  -c, --config <path>  Specify initial configuration file path.
  -h, --help           Show this help message.
  -i, --init           Create an initial empty GlobalConfig.json file.
```

## Working with Plugins
### Generating a New Plugin
To create a new plugin:
```sh
cd plugins
python3 createNewPluginWithName.py [PluginName]
```
This generates a new plugin template inside the `plugins/` directory.

### Using Plugins
1. Ensure your compiled plugin is inside `build/plugins/`.
2. Add the plugin to `dummyConfig.json` under the `"plugins"` array.
3. Plugins must implement the `PluginInterface.hpp` API.

## Web Interface
### Launching the Web UI
Once Inlook is running, open your browser and navigate to:

http://localhost:8080

(Hostname/port can be changed in `config/GlobalConfig.json`)

### Web UI Features
- Plugins Tab: View and manage loaded plugins.
- Emails Tab: Browse and analyze parsed emails.
- Logs Tab: View system logs in real-time.
- Config Tab: Edit system and workflow configurations.
- Docs Tab: View internal documentation (Doxygen & README.md).

## Project Structure
```
inlook/
│── include/                # Inlook_Core header files (linked to all plugins)
│   ├── *.hpp
│── src/                    # Inlook_Core source files (shared library)
│   ├── *.cpp
│── plugins/                # Plugins directory
│   ├── DummyPlugin/        # An example plugin
│   ├── newNamedPlugin.py   # Python script for generating new plugins. 
│── config/                 # Configuration files
│   │── workflows/          # Directory containing your workflows.
│   │── GlobalConfig.json   # Default global config .json file.
│── web/                    # Web UI files
│── docs/                   # (optional) Doxygen files (copied to binary dir)
│── build/                  # Compilation output
```

## Developing a New Plugin

**see plugins/README.md for more information**

1. Use the provided script to generate a new plugin template:
   ```sh
   cd plugins
   python3 newNamedPlugin.py [PluginName]
   ```
   This will create a new plugin with the specified name, replacing placeholders accordingly.
2. Modify the **plugin schema** to define expected config attributes. The schema file is located within the source (.cpp) file and defines how the plugin interacts with the system, as well as aids UI configuration.
3. Update the **plugin name, description, and metadata** to reflect its purpose. Ensure the plugin class name and filenames match your chosen plugin name.
4. Add your plugin to the **config file** (created with `-i`) by adding a new entry in the `"plugins"` array.
5. Implement the **constructor and `execute()` function**, ensuring that it processes emails correctly and adheres to the defined schema.
6. Compile and run Inlook with your updated configuration to test the new plugin.

## API & WebSockets
Inlook provides a REST API and WebSocket interface for interaction. The API documentation is available under the Docs tab in the Web UI.

## Contributing
This project is very new, and has only been worked on internally thus far. If you have any thoughts or concerns, please feel free to contact us.
Contributions are also welcome. If you find an issue or want to contribute a feature, please open a pull request or create an issue.

## Disclaimer
This tool is intended for research and development purposes only. Misuse of this tool is strictly prohibited.
