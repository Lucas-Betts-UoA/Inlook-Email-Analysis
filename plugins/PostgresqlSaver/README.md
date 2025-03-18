


# PostgresqlSaver Plugin

## Overview

The PostgresqlSaver plugin is designed to save email data into a PostgreSQL database. It's part of a larger email processing system and handles the storage of emails, their headers, body parts, and associated attributes.

## Features

- Saves emails and their components to a PostgreSQL database
- Supports both standard emails and MIME multipart emails
- Handles email headers, body parts, and custom attributes
- Clears existing data before inserting new data

## Configuration

To use the PostgresqlSaver plugin, you need to include it in your configuration file. Here's an example of how to configure the plugin:

```json
{
  "name": "PostgresqlSaver",
  "options": {
    "databasePath": "postgresql://postgres:myPassword@localhost/emailtestdatabase",
    "schemaName": "emailschema",
    "datasetName": "Untroubled.org",
    "datasetDescription": "This directory contains all the spam that I have received since early 1998. I have employed various \"bait\" addresses, such as <bait@em.ca> to trick email address harvesters into putting them on spam lists."
  }
}
```

### Configuration Options

- `databasePath`: The connection string for your PostgreSQL database. Format: `postgresql://username:password@host/database`
- `schemaName`: The name of the schema in your PostgreSQL database where the tables are located
- `datasetName`: A unique name for the dataset being processed
- `datasetDescription`: A description of the dataset

## Database Schema

The plugin expects the following tables to exist in the specified schema:

- `dataset`
- `email`
- `emailheaderkey`
- `emailheaderval`
- `emailpart`
- `emailpartheaderkey`
- `emailpartheaderval`
- `attributebag`

Ensure these tables are created with the appropriate structure before running the plugin.

## Usage

1. Include the plugin configuration in your main configuration file.
2. Ensure your PostgreSQL database is set up with the correct schema and tables.
3. The plugin will automatically clear existing data and insert new data when executed.

## Important Notes

- The plugin clears all existing data in the specified tables before inserting new data. Be cautious when using this in production environments.
- Make sure the database user specified in the `databasePath` has the necessary permissions to truncate and insert data into the tables.
- The plugin uses the `pqxx` library for PostgreSQL operations. Ensure this library is properly installed and linked in your project.

## Error Handling

- The plugin logs errors and exceptions during database operations.
- If an error occurs during execution, the plugin will return -1, indicating a failure.

## Testing

The plugin includes a `runTests()` method, which currently returns 0. Implement specific tests as needed for your use case.

## Dependencies

- PostgreSQL database
- pqxx library for C++ PostgreSQL client API

Ensure all dependencies are properly installed and configured in your development and production environments.

