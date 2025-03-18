# AttributeBagStringAdder Plugin

## Description
The `AttributeBagStringAdder` plugin is designed to add specific attributes to emails in an `EmailList`. It retrieves attribute key-value pairs from the plugin configuration and inserts them into each email's attribute bag.

## Usage

### Configuration
To use the `AttributeBagStringAdder` plugin, add it to your configuration file as follows:

```json
{
  "name": "AttributeBagStringAdder",
  "options": {
    "attributes": [
      {
        "attributeKey": "[key to be added]",
        "attributeVal": "[value to be added]"
      }
    ]
  }
}
```

### Attribute Options

- `attributeKey`: Specifies the key of the attribute to be added to each email.
- `attributeVal`: Specifies the value of the attribute to be added.

### Example Configurations

1. Add a custom attribute to all emails:
```json
{
  "name": "AttributeBagStringAdder",
  "options": {
    "attributes": [
      {
        "attributeKey": "customKey",
        "attributeVal": "customValue"
      }
    ]
  }
}
```

2. Add multiple attributes to emails:
```json
{
  "name": "AttributeBagStringAdder",
  "options": {
    "attributes": [
      {
        "attributeKey": "key1",
        "attributeVal": "value1"
      },
      {
        "attributeKey": "key2",
        "attributeVal": "value2"
      }
    ]
  }
}
```

## Functionality
The `AttributeBagStringAdder` plugin processes each email in the `EmailList` and adds the specified attributes from the configuration. Each attribute is inserted using its key-value pair.

### Workflow:
1. The plugin reads the `attributes` array from its configuration.
2. For each email in the `EmailList`, it inserts all specified attributes into the email's attribute bag.

## Notes
- Ensure that the `attributes` array in your configuration contains valid JSON objects with both `attributeKey` and `attributeVal`.
- The plugin does not overwrite existing attributes with the same key; it will simply insert the new attributes as specified.
- This plugin is designed to run after any plugins that generate or modify emails but before plugins that analyze or process attributes.

## Integration
Place the `AttributeBagStringAdder` plugin in your plugin chain where attributes need to be added before further processing. For example:
1. Plugins that load or generate emails.
2. **`AttributeBagStringAdder`** (to add attributes).
3. Plugins that analyze or process emails based on attributes.

## Testing
The plugin includes a basic test method (`runTests`) that can be used to verify its functionality. This method currently returns a success code (`0`) and logs a message indicating it was called.