# PostgresqlReader Plugin

## Overview

The PostgresqlReader plugin is designed to read email data from a PostgreSQL database. It's an integral part of an email processing system, handling the retrieval of emails, their headers, body parts, and associated attributes.

## Features

- Reads emails and their components from a PostgreSQL database
- Supports both standard emails and MIME multipart emails
- Handles email headers, body parts, and custom attributes
- Provides filtering options for email retrieval
- Ensures uniqueness of emails in the EmailList

## Configuration

The PostgresqlReader plugin uses the following JSON schema for configuration:

```json
{
  "databasePath": "string",
  "schemaName": "string",
  "datasetName": "string",
  "filters": [
    {
      "columnName": "string",
      "condition": "string",
      "value": "string"
    }
  ]
}
```

### Configuration Options

- `databasePath`: Connection string for the PostgreSQL database
- `schemaName`: Name of the schema containing email-related tables
- `datasetName`: Name of the dataset to read from
- `filters`: Array of filter objects to apply when retrieving emails (optional)

## Database Schema

The plugin expects the following tables in the specified schema:

- `dataset`
- `email`
- `emailheaderkey`
- `emailheaderval`
- `emailpart`
- `emailpartheaderkey`
- `emailpartheaderval`
- `attributebag`

## Functionality

1. Connects to the specified PostgreSQL database
2. Sets the search path to the specified schema
3. Retrieves emails based on the dataset name and optional filters
4. For each email:
    - Reads headers and their values
    - Retrieves attributes
    - Processes the email body (standard or MIME multipart)
    - Ensures email uniqueness before adding to the EmailList
5. Handles character encoding, converting content to UTF-8

## Error Handling

- Logs errors and exceptions during database operations
- Sets plugin state to "FAILED" if an exception occurs during execution

## Charset Handling

- Attempts to convert email bodies and attributes to UTF-8
- Logs errors if conversion fails and skips the problematic email

## Dependencies

- PostgreSQL database
- pqxx library for C++ PostgreSQL client API
- Custom classes: Email, EmailList, EmailBody, MIMEMultipartBodies, StandardEmailBody
- AttributeBagRegistry for attribute deserialization

## Usage

1. Include the plugin in your configuration with the necessary options
2. Ensure the PostgreSQL database is set up with the correct schema and tables
3. The plugin will populate an EmailList object when executed

## Notes

- The plugin generates a unique hash for each email to prevent duplicates
- It supports both standard and MIME multipart email structures
- Custom attributes are deserialized using the AttributeBagRegistry

Ensure all dependencies are properly installed and configured in your development and production environments.
