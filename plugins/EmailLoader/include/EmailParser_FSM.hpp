#pragma once
#include <filesystem>
#include <fstream>
#include <functional>
#include <map>
#include <regex>
#include <string>
#include <unordered_map>
#include <vector>
#include "AttributeBagValueInterface.hpp"
#include "Email.hpp"
#include "EmailBody.hpp"
#include "EmailListView.hpp"
#include "EmailLoaderAttributes.hpp"
#include "fasttext.h"

#include <unicode/ucnv.h> // ICU4C converter
#include <unicode/ucsdet.h> // ICU4C detector

class EmailParser_FSM {
public:
    using StateHandler = std::function<int(const std::string&)>;

    // Constructor
    EmailParser_FSM(EmailListView* newEmailList);

    // Recursively parse a directory of emails or a single email
    void parse(std::filesystem::path& p);

private:
    // Member variables
    bool isMultipart = false;
    std::string boundary;
    std::stringstream headervalstringstream;
    std::stringstream headerkey;
    std::stringstream body;
    std::stringstream mimebody;
    std::stringstream mimeheadervalstringstream;
    std::stringstream mimeheaderkey;
    std::pmr::map<std::string, std::vector<std::string>> mimeheadermap;
    Email emailObj;
    std::unique_ptr<EmailBody> emailBodyObj;
    EmailListView* emailList;
    fasttext::FastText fasttext;


    enum class ReadingState {
        NotReading,
        Header,
        EmailPartBody,
        MIMEMultiPartHeader,
        MIMEMultiPartBody
    };
    ReadingState currentState;
    std::unordered_map<ReadingState, StateHandler> stateHandlers;

    // Member functions
    void processLine(const std::string& line);
    void readEmail(const std::string& filename);
    void checkIfMIME(const std::string& line);

    // State handler methods
    int handleNotReading(const std::string& input);
    int handleHeader(const std::string& input);
    int handleEmailPartBody(const std::string& input);
    int handleMIMEMultiPartHeader(const std::string& input);
    int handleMIMEMultiPartBody(const std::string& input);

    // Detecting and converting encoding
    std::vector<char> readFileAsBytes(const std::string& filePath);
    std::string detectFileEncoding(const std::vector<char>& buffer);
    std::string convertToUTF8(const std::vector<char>& inputBuffer, const std::string& encoding);
    void detectLanguage(const std::string& text);
    std::string getLanguageName(const std::string& isoCode, const std::string& displayLocale = "en");

    void flush();
    void resetMemberVars();

    // Helper method to change state
    void changeState(ReadingState newState);
};

//EMAILPARSER_FSM_HPP
