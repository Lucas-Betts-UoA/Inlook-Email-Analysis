# EmailListFilter Plugin

## Description
The **EmailListFilter** plugin filters emails in an `EmailList` using configurable criteria. It supports complex filtering operations on headers, attributes, body content, and MIME parts using exact matches or regular expressions.

---

## Configuration Schema
```json
{
  "filters": [
    {
      "fields": [
        {
          "value": "<field-type>",
          "outcome": "<include/exclude>",
          "filterBy": "<string/regex>",
          "filterVals": [
            {"filterValue": "<match-value>"}
          ]
        }
      ]
    }
  ]
}
```

### Key Configuration Elements

| Property | Type | Description |  
|----------|------|-------------|  
| `filters` | Array | List of filter groups to apply sequentially |  
| `fields` | Array | Filter criteria to apply **within a single processing operation** |  
| `value` | String | Field type to filter (`headerKey`, `headerVal`, `attributeKey`, `attributeVal`, `body`, `MIMEPartKey`, `MIMEPartVal`) |  
| `outcome` | String | `include` keeps matching emails, `exclude` removes them |  
| `filterBy` | String | Matching method: `string` (exact match) or `regex` |  
| `filterVals` | Array | Values to match against (using `filterValue` keys) |  

---

## Execution Logic
1. Processes emails in reverse order (to safely remove items during iteration)
2. Extracts these data points for filtering:
    - Email headers (keys + values)
    - Custom attributes (keys + stringified values)
    - Body content (supports standard + MIME multipart bodies)
    - MIME part headers (keys + values)
3. Applies filters sequentially as defined in configuration
4. Removes emails when:
    - `include` outcome with **no matches**
    - `exclude` outcome with **any match**

---

## Example Configurations

### 1. Include emails with specific header value
```json
{
  "name": "EmailListFilter",
  "options": {
    "filters": [
      {
        "fields": [
          {
            "value": "headerVal",
            "outcome": "include",
            "filterBy": "string",
            "filterVals": [
              {"filterValue": "urgent"}
            ]
          }
        ]
      }
    ]
  }
}
```

### 2. Exclude emails with test-related attributes
```json
{
  "name": "EmailListFilter",
  "options": {
    "filters": [
      {
        "fields": [
          {
            "value": "attributeKey",
            "outcome": "exclude",
            "filterBy": "regex",
            "filterVals": [
              {"filterValue": ".*test.*"}
            ]
          }
        ]
      }
    ]
  }
}
```

### 3. Combine multiple filters
```json
{
  "name": "EmailListFilter",
  "options": {
    "filters": [
      {
        "fields": [
          {
            "value": "headerKey",
            "outcome": "include",
            "filterBy": "string",
            "filterVals": [
              {"filterValue": "X-Priority"}
            ]
          },
          {
            "value": "body",
            "outcome": "exclude",
            "filterBy": "regex",
            "filterVals": [
              {"filterValue": "\\b(viagra|cialis)\\b"}
            ]
          }
        ]
      }
    ]
  }
}
```

---

## Technical Notes
1. **Reverse Iteration**: Processes emails from last to first for safe removal
2. **Regex Handling**: Uses full string matching (`^pattern$` implied)
3. **Multipart Bodies**: Automatically unpacks MIME parts for filtering
4. **Attribute Values**: Converts all attribute values to strings for matching

---

## Filtering Capabilities

| Field | Target Data | Example Use Cases |  
|-------|-------------|-------------------|  
| `headerKey` | Header names | Filter by `From`, `Subject`, custom headers |  
| `headerVal` | Header content | Find specific senders, subjects |  
| `attributeKey` | Custom attribute keys | Filter by system-generated tags |  
| `attributeVal` | Attribute values | Filter by classification results |  
| `body` | Email content | Content-based filtering |  
| `MIMEPartKey` | MIME part headers | Filter attachments by `Content-Type` |  
| `MIMEPartVal` | MIME header values | Find specific file types |  

---

## Best Practices
1. Place this plugin **after** email loading/parsing stages
2. Use `include` filters before `exclude` filters in complex workflows
3. Test regex patterns with [regex101.com](https://regex101.com/) (ECMAScript flavor)
4. For combined filters, use separate filter groups rather than multiple fields

---

## Diagnostic Tips
1. Check plugin state transitions via `printRecursiveInstanceTreeJson`
2. Verify filter patterns match email casing (case-sensitive)
3. Monitor removed email count via logger (`LOG_INFO` level)
