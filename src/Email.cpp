#include "Email.hpp"

Email::Email() : body(nullptr), isMIMEMultipart(false), uniqueHash(0) {}

Email::Email(const Email& other) :
    header(other.header), isMIMEMultipart(other.getIsMIMEMultipart()), uniqueHash(other.getUniqueHash()) {
    if (other.body) {
        if (auto *standardBody = dynamic_cast<StandardEmailBody *>(other.body.get())) {
            body = std::make_unique<StandardEmailBody>(*standardBody);
        } else if (auto *multiPartBody = dynamic_cast<MIMEMultipartBodies *>(other.body.get())) {
            body = std::make_unique<MIMEMultipartBodies>(*multiPartBody);
        } else {
            throw std::runtime_error("Unknown EmailBody subclass during copy construction");
        }
    }

    for (const auto &[key, value]: other.attribute_bag) {
        attribute_bag[key] = std::unique_ptr<AttributeBagValueInterface>(value->clone());
    }
}

Email& Email::operator=(Email&& other) noexcept {
    if (this != &other) {
        header = std::move(other.header);
        attribute_bag = std::move(other.attribute_bag);
        body = std::move(other.body);
    }
    return *this;
}

nlohmann::json Email::toJson() {
    nlohmann::json emailJson;
    try {
        emailJson["uniqueHash"] = getUniqueHash();

        emailJson["isMIMEMultipart"] = isMIMEMultipart;

        emailJson["headers"] = header;

        emailJson["body"] = body ? body->getAllBodyData() : nullptr;

        emailJson["attributes"] = nlohmann::json::object();
        for (const auto& [key, valuePtr] : attribute_bag) {
            emailJson["attributes"][key] = valuePtr ? valuePtr->serializeToString() : nullptr;
        }
    } catch (const std::exception& e) {
        LOG_ERROR << "Failing " << e.what();
    }
    return emailJson;
}

void Email::setHeader(const std::string& key, const std::string& value) {
    header[key] = value;
}

std::map<std::string, std::string> Email::getHeader() const {
    return header;
}

std::vector<std::string> Email::getHeaderKeys() const {
    std::vector<std::string> keys;
    for (const auto& imap : header) {
        keys.push_back(imap.first);
    }
    return keys;
}

std::vector<std::string> Email::getHeaderValues() const {
    std::vector<std::string> values;
    for (const auto &imap : header) {
        values.push_back(imap.second);
    }
    return values;
}

void Email::setBody(std::unique_ptr<EmailBody> newBody) {
    body = std::move(newBody);
}

EmailBody* Email::getBody() const {
    return body.get();
}

std::vector<std::string> Email::getAttributeKeys() const {
    std::vector<std::string> attributeKeys;
    for (const auto &imap : attribute_bag)
        attributeKeys.push_back(imap.first);
    return attributeKeys;
}

std::vector<AttributeBagValueInterface*> Email::getAttributeValues() const {
    std::vector<AttributeBagValueInterface*> attributeValues;
    for (const auto& imap : attribute_bag)
        attributeValues.push_back(imap.second.get());
    return attributeValues;
}

AttributeBagValueInterface* Email::getAttributeValue(const std::string& key) const {
    return attribute_bag.at(key).get();
}

void Email::insertAttribute(const std::string& key, std::unique_ptr<AttributeBagValueInterface> attribute) {
    attribute_bag[key] = std::move(attribute);
}

void Email::setIsMIMEMultipart(bool value) {
    this->isMIMEMultipart = value;
}

bool Email::getIsMIMEMultipart() const {
    return this->isMIMEMultipart;
}

void Email::generateUniqueHash() {
    std::hash<std::string> hasher;
    uniqueHash = hasher(getAttributeValue("File bytes")->toString());
}

size_t Email::getUniqueHash() const {
    return uniqueHash;
}

bool Email::operator==(const Email& other) const {
    return uniqueHash == other.uniqueHash;
}