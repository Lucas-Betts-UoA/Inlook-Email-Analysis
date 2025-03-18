# AttributeBagLogger Plugin

## Overview

The AttributeBagLogger plugin is designed to log email attributes from an EmailList. It can either log attributes for all emails or selectively log attributes for specific emails based on file identifiers.

## Features

- Logs email attributes to the console
- Option to filter logging by specific file identifiers
- Handles exceptions when converting attribute values to strings
- Skips logging of "File bytes" attribute to avoid large data dumps

## Configuration

The plugin uses the following JSON schema for configuration:

```json
{
  "logByFile": "boolean",
  "files": [
    {
      "file": "string"
    }
  ]
}
```

### Configuration Options

- `logByFile`: Boolean flag to determine if logging should be filtered by specific files
- `files`: Array of file objects, each containing a "file" property with a file identifier (required if `logByFile` is true)

## Usage

1. Include the plugin in your configuration file:

```json
{
  "name": "AttributeBagLogger",
  "options": {
    "logByFile": false
  }
}
```

2. To log attributes for specific files:

```json
{
  "name": "AttributeBagLogger",
  "options": {
    "logByFile": true,
    "files": [
      {"file": "example1.eml"},
      {"file": "example2.eml"}
    ]
  }
}
```

## Functionality

1. Iterates through emails in the provided EmailList
2. If `logByFile` is true, only logs attributes for emails matching the specified file identifiers
3. Logs each attribute key-value pair, except for "File bytes"
4. Handles exceptions when converting attribute values to strings
5. Logs the attribute type if an exception occurs during conversion

## Error Handling

- Logs errors if exceptions occur during attribute value conversion
- Continues processing other attributes if an error occurs with one

## Dependencies

- Custom classes: EmailList, Email, PluginRunnableInterface
- Logger functionality (LOG_INFO, LOG_ERROR)
- nlohmann::json for JSON handling

## Notes

- The plugin state transitions from "LOADED" to "READY" to "RUNNING" to "COMPLETE" during execution
- Attribute logging uses the INFO log level
- Errors are logged using the ERROR log level

Ensure all dependencies are properly set up in your development and production environments.