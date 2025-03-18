#pragma once
#include <string>
#include <sstream>

class EmailBody {
public:
    virtual ~EmailBody() = default;
    virtual std::string getAllBodyData() = 0;
};

class StandardEmailBody final : public EmailBody {
private:
    std::string content;
public:
    StandardEmailBody() = default;
    explicit StandardEmailBody(const std::string& content) : content(content) {}
     std::string getAllBodyData() {return content;}
    void setContent(const std::string& newContent) {content = newContent;}
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
     std::vector<MIMEMultipartPart> getMultipartParts() const {
        return multipartBodies;
    }
};