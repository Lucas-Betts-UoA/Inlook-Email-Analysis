#pragma once
#include <string>
#include <sstream>

class EmailBody {
public:
    virtual ~EmailBody() = default;
    virtual std::string getAllBodyData() = 0;
    virtual nlohmann::json toJson() const = 0;
    static std::unique_ptr<EmailBody> fromJson(const nlohmann::json& json);
};

class StandardEmailBody final : public EmailBody {
private:
    std::string content;
public:
    StandardEmailBody() = default;
    explicit StandardEmailBody(const std::string& content) : content(content) {}
    std::string getAllBodyData() {return content;}
    void setContent(const std::string& newContent) {content = newContent;}
    nlohmann::json toJson() const override{
        return {
                {"type", "standard"},
                {"content", content}
        };
    }
};



class MIMEMultipartPart {
private:
    std::pmr::map<std::string, std::vector<std::string>> header;
    std::string content;
public:
    MIMEMultipartPart() = default;
    explicit MIMEMultipartPart(const std::string& content) : content(content) {}
     std::string getBody() const {return content;}
     std::pmr::map<std::string, std::vector<std::string>> getHeader() const {return header;}
     std::vector<std::string> getHeaderKeys() const {
        std::vector<std::string> keys;
        for (const auto& imap : header) {
            keys.push_back(imap.first);
        }
        return keys;
    }
     std::vector<std::string> getHeaderValues() const {
        std::vector<std::string> values;
        for (const auto& imap : header) {
            for (const auto& key : imap.second) {
                values.push_back(key);
            }
        }
        return values;
    }
    void addMimePart(const std::pmr::map<std::string, std::vector<std::string>>& newHeader, const std::string& newContent) {
        header = newHeader;
        content = newContent;
    };
    std::string getMimePartHeader() {
        std::stringstream headerStream;
        for (auto it = header.begin(); it != header.end(); ++it) {
            headerStream << it->first << ": ";
            for (auto it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
                headerStream << *it2;
            }
            headerStream << "\r\n";
        }
        return headerStream.str();
    }
};

class MIMEMultipartBodies final : public EmailBody {
private:
    std::vector<MIMEMultipartPart> multipartBodies;
public:
    MIMEMultipartBodies() = default;
    void addPart(const std::pmr::map<std::string, std::vector<std::string>>& newHeader, const std::string& newContent) {
        MIMEMultipartPart part;
        part.addMimePart(newHeader, newContent);
        multipartBodies.push_back(part);
    }
    std::string getAllBodyData() {
        std::stringstream MIMEBodyData;
        for (MIMEMultipartPart& multipartBody : multipartBodies) { // Appends each MIMEMultipart body to a single stringstream.
            MIMEBodyData << multipartBody.getMimePartHeader() << "\r\n"; // Simply appends the headers.
            MIMEBodyData << multipartBody.getBody();

        }
        return MIMEBodyData.str();
    }
    nlohmann::json toJson() const override {
        nlohmann::json parts = nlohmann::json::array();

            for (const auto& part : multipartBodies) {
                nlohmann::json headers;
                for (const auto& [k, v] : part.getHeader()) {
                    headers[k] = v;
                }

                parts.push_back({
                    {"headers", headers},
                    {"content", part.getBody()}
                });
            }

            return {
                        {"type", "mime"},
                        {"parts", parts}
        };
    }
     std::vector<MIMEMultipartPart> getMultipartParts() const {
        return multipartBodies;
    }
};



inline std::unique_ptr<EmailBody> EmailBody::fromJson(const nlohmann::json& json) {
    if (!json.contains("type")) {
        throw std::runtime_error("EmailBody JSON missing 'type'");
    }

    const std::string type = json.at("type").get<std::string>();

    if (type == "standard") {
        return std::make_unique<StandardEmailBody>(json.value("content", ""));
    }

    if (type == "mime") {
        auto mimeBody = std::make_unique<MIMEMultipartBodies>();
        for (const auto& part : json.at("parts")) {
            std::pmr::map<std::string, std::vector<std::string>> headers;
            for (auto& [key, values] : part.at("headers").items()) {
                headers[key] = values.get<std::vector<std::string>>();
            }
            mimeBody->addPart(headers, part.value("content", ""));
        }
        return mimeBody;
    }
    throw std::runtime_error("Unknown EmailBody type: '" + type + "'");
}


