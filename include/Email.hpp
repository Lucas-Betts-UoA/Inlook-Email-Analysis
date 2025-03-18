#pragma once

#include <any>
#include <string>
#include <map>
#include <utility>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>
#include <sstream>
#include <Logger.hpp>
#include "EmailBody.hpp"
#include "AttributeBagValueInterface.hpp"
#include <nlohmann/json.hpp>

/**
 * @class Email
 * @brief Represents an email with headers, body, and attributes.
 *
 * The Email class encapsulates the components of an email message, including the headers,
 * body, and additional attributes. It supports setting and retrieving headers and body content,
 * managing attributes via an attribute bag to allow plugin-generated email-specific data.
 */
class Email {
public:
    /**
     * @brief Default constructor for the Email class.
     *
     * Initializes the Email object with a null body.
     */
    Email();

    /**
     * @brief Default destructor for the Email class.
     */
    ~Email() = default;

    /**
     * @brief Copy constructor.
     *
     * Creates a deep copy of the given Email object, preserving the the unique_ptr<EmailBody> variable.
     *
     * @param other The Email object to copy from.
     */
    Email(const Email& other);

    /**
     * @brief Deleted copy assignment operator.
     *
     * Copy assignment is not allowed for the Email class to prevent unintended copying of the
     * unique_ptr<EmailBody> variable.
     */
    Email& operator=(const Email&) = delete;

    /**
     * @brief Move constructor.
     *
     * Defaulted move constructor for the Email class.
     */
    Email(Email&& other) noexcept = default;

    /**
     * @brief Move assignment operator.
     *
     * Moves the content of another Email object to this one. Created for the unique_ptr<EmailBody> variable.
     *
     * @param other The Email object to move from.
     * @return Reference to this Email object.
     */
    Email& operator=(Email&& other) noexcept;

    /**
     * @brief Jsonify an entire email.
     *
     * @return nlohmann::json which represents the entire email.
     */
    nlohmann::json toJson();

    /**
     * @brief Sets a header field.
     *
     * Adds or updates a header field with the given key and value.
     *
     * @param key The header field name.
     * @param value The header field value.
     */
    void setHeader(const std::string& key, const std::string& value);

    /**
     * @brief Retrieves all header fields.
     *
     * @return A map of header fields with their corresponding values.
     */
    std::map<std::string, std::string> getHeader() const;

    /**
     * @brief Retrieves all header keys.
     *
     * @return A vector of strings containing all header keys.
     */
    std::vector<std::string> getHeaderKeys() const;

    /**
     * @brief Retrieves all header values.
     *
     * @return A vector of strings containing all header values.
     */
    std::vector<std::string> getHeaderValues() const;

    /**
     * @brief Sets the body of the email.
     *
     * Assigns a new EmailBody to the email.
     *
     * @param newBody A unique pointer to the new EmailBody.
     */
    void setBody(std::unique_ptr<EmailBody> newBody);

    /**
     * @brief Retrieves the email body.
     *
     * @return A pointer to the EmailBody of the email.
     */
    EmailBody* getBody() const;

    /**
     * @brief Retrieves the list of attribute keys.
     *
     * @return A vector containing the keys of the attributes in the attribute bag.
     */
    std::vector<std::string> getAttributeKeys() const;

    /**
     * @brief Retrieves all attribute values.
     *
     * @return A vector of pointers to AttributeBagValueInterface.
     */
    std::vector<AttributeBagValueInterface*> getAttributeValues() const;

    /**
     * @brief Retrieves the value of a specific attribute.
     *
     * @param key The key of the attribute to retrieve.
     * @return The value of the attribute as a pointer to AttributeBagValueInterface.
     * @throws std::out_of_range if the key is not found.
     */
    AttributeBagValueInterface* getAttributeValue(const std::string& key) const;

    /**
     * @brief Inserts an attribute into the attribute bag.
     *
     * Associates a key with an attribute in the attribute bag.
     *
     * @param key The key for the attribute.
     * @param attribute The attribute value.
     */
    void insertAttribute(const std::string& key, std::unique_ptr<AttributeBagValueInterface> attribute);

    /**
     * @brief Sets the MIME multipart status of the email.
     *
     * @param value True if the email is MIME multipart; otherwise, false.
     */
    void setIsMIMEMultipart(bool value);

    /**
     * @brief Checks if the email is a MIME multipart message.
     *
     * @return True if the email is MIME multipart; otherwise, false.
     */
    bool getIsMIMEMultipart() const;

    /**
     * @brief Generates a unique hash for the email.
     *
     * Creates a hash based on the email's headers and body content.
     * This method should be called after setting the email's content.
     */
    void generateUniqueHash();

    /**
     * @brief Retrieves the unique hash of the email.
     *
     * @return The unique hash value as a size_t.
     */
    size_t getUniqueHash() const;

    /**
     * @brief Checks if two Emails are the same based on their hash values.
     *
     * @return True if the Emails have the same hash value; otherwise false.
     */
    bool operator==(const Email& other) const;

private:
    std::map<std::string, std::string> header;
    std::unique_ptr<EmailBody> body;
    std::pmr::unordered_map<std::string, std::unique_ptr<AttributeBagValueInterface>> attribute_bag;
    bool isMIMEMultipart;
    size_t uniqueHash;
};