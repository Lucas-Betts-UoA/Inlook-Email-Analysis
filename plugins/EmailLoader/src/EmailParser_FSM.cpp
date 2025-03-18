#include <EmailParser_FSM.hpp>
#include "fasttext.h"
#include <unicode/uloc.h>
#include <unicode/ustring.h>
const std::regex headerkeyRegex(R"(^([\w-]+): (.*))");
const std::regex multipartRegex(R"(\s*multipart/.*?boundary=\"?([^\";\s]+)\"?)");
const std::regex mimeheaderkeyRegex(R"(^([\w-]+): (.*))");

using StateHandler = std::function<int(const std::string&)>;
EmailParser_FSM::EmailParser_FSM(EmailListView* newEmailList) : currentState(ReadingState::NotReading) {
    // resetMemberVars();
    // Initialize state handlers
    stateHandlers[ReadingState::NotReading] = [this](const std::string& input) -> int {return handleNotReading(input);};
    stateHandlers[ReadingState::Header] = [this](const std::string& input) -> int  {return handleHeader(input); };
    stateHandlers[ReadingState::MIMEMultiPartHeader] = [this](const std::string& input) -> int {return handleMIMEMultiPartHeader(input); };
    stateHandlers[ReadingState::EmailPartBody] = [this](const std::string& input) -> int {return handleEmailPartBody(input); };
    stateHandlers[ReadingState::MIMEMultiPartBody] = [this](const std::string& input) -> int {return handleMIMEMultiPartBody(input); };
    emailList = newEmailList;
    try {
        fasttext.loadModel("lid.176.bin");
    } catch (std::exception& e) {
        throw std::runtime_error("Error loading fastText lid.176.bin model");
    }

}

// recursively parse a directory of emails, or a single email.
void EmailParser_FSM::parse(std::filesystem::path &p) {
    std::filesystem::file_status s = status(p);
    switch (s.type()) {
        case std::filesystem::file_type::regular:
        {
            //LOG_DEBUG_VERBOSE << "Loading Email: " << p.c_str();
            emailObj.insertAttribute("File identifier", std::make_unique<AttributeBagString>(AttributeBagString(p.string())));
            readEmail(p.string());
            break;
        }

        case std::filesystem::file_type::directory:
        {
            //LOG_DEBUG_VERBOSE << "Entering New Director: " << p.c_str();
            for (std::filesystem::path dir_entry : std::filesystem::directory_iterator{p}) {
                parse(dir_entry);
            }

            break;
        }

        default:
            LOG_WARNING << "Unknown file type: " <<  static_cast<int>(s.type());
    }
}

void EmailParser_FSM::processLine(const std::string& line) {
    if (stateHandlers.find(currentState) != stateHandlers.end()) {
        int lineprocessed = stateHandlers[currentState](line);
        while (lineprocessed != 1) {
            lineprocessed = stateHandlers[currentState](line);
        }
    } else {
        LOG_ERROR << "No handler for current state";
    }
}

void EmailParser_FSM::readEmail(const std::string& filePath) { // throws std::exception();
    try {
        std::vector<char> fileBytes = readFileAsBytes(filePath);
        emailObj.insertAttribute("File bytes", std::make_unique<AttributeBagCharVector>(AttributeBagCharVector(fileBytes)));
        std::string encoding = detectFileEncoding(fileBytes);
        if (encoding != "UNKNOWN"){
            std::string utf8Text = convertToUTF8(fileBytes, encoding);
            std::istringstream iss(utf8Text);
            detectLanguage(utf8Text);
            for (std::string line; std::getline(iss, line);) {
                processLine(line);
            }
        } else {
            throw "Could not detect file encoding.";
        }

        flush();
    } catch (std::exception &e) {
        LOG_ERROR << "Error while reading file: " << filePath << " with error: " << e.what();
    }
}

std::string EmailParser_FSM::getLanguageName(const std::string& isoCode, const std::string& displayLocale ) {
    UErrorCode status = U_ZERO_ERROR;
    UChar langName[128]; // UTF-16 buffer

    // Retrieve language name in UTF-16 format
    uloc_getDisplayLanguage(isoCode.c_str(), displayLocale.c_str(), langName, sizeof(langName)/sizeof(UChar), &status);
    if (U_FAILURE(status)) {
        LOG_ERROR << "Error getting language name for: " << isoCode;
        return "Error getting language name for: " + isoCode;
    }

    // Convert UTF-16 UChar* to UTF-8 std::string
    char utf8LangName[128];
    int32_t utf8Len = 0;
    u_strToUTF8(utf8LangName, sizeof(utf8LangName), &utf8Len, langName, -1, &status);

    if (U_FAILURE(status)) {
        LOG_ERROR << "Error converting language name to UTF-8.";
        return "Error converting language name to UTF-8.";
    }

    return std::string(utf8LangName);
}

void EmailParser_FSM::detectLanguage(const std::string& text) {
    std::istringstream file(text);
    int num_predictions = 2; // Number of top predictions to return


    // Load the pre-trained language detection model

    std::vector<int32_t> words;
    std::string word;
    while (file >> word) {
        int32_t wordId = fasttext.getWordId(word);
        if (wordId >= 0) {
            words.push_back(wordId);
        }
    }

    // Predict language for the entire file
    fasttext::Predictions predictions;
    fasttext.predict(num_predictions, words, predictions);
    std::vector<std::pair<std::string, float>> languages;
    // Convert predictions to the format used in printPredictions
    for (const auto& p : predictions) {
        std::string langLabel = fasttext.getDictionary()->getLabel(p.second);
        if (fasttext.getDictionary()->getLabel(p.second).length() == 11) {
            std::string langName = getLanguageName(langLabel.substr(langLabel.length()-2), "en");
            languages.push_back(std::make_pair(langName, std::exp(p.first)));
            //LOG_DEBUG_VERBOSE << "Prediction: " << langName;
            //LOG_DEBUG_VERBOSE << "Probability: " << std::exp(p.first);
        } else if (fasttext.getDictionary()->getLabel(p.second).length() == 12) {
            std::string langName = getLanguageName(langLabel.substr(langLabel.length()-3), "en");
            languages.push_back(std::make_pair(langName, std::exp(p.first)));
            //LOG_DEBUG_VERBOSE << "Prediction: " << langName;
            //LOG_DEBUG_VERBOSE << "Probability: " << std::exp(p.first);
        }
    }
    emailObj.insertAttribute("Language predictions", std::make_unique<AttributeBagStringFloatPairVector>(AttributeBagStringFloatPairVector(languages)));
}

std::vector<char> EmailParser_FSM::readFileAsBytes(const std::string &filePath) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Error: Unable to open file: " + filePath);
    }

    // Read file into a byte vector
    return std::vector<char>((std::istreambuf_iterator<char>(file)),
                              std::istreambuf_iterator<char>());
}

std::string EmailParser_FSM::detectFileEncoding(const std::vector<char> &buffer) {
    UErrorCode status = U_ZERO_ERROR;
    UCharsetDetector* detector = ucsdet_open(&status);
    if (U_FAILURE(status)) {
        throw std::runtime_error("Error: ICU4C charset detector initialization failed.");
    }

    // Feed file data into ICU4C detector
    ucsdet_setText(detector, buffer.data(), buffer.size(), &status);

    // Detect encoding
    const UCharsetMatch* match = ucsdet_detect(detector, &status);
    if (U_FAILURE(status) || match == nullptr) {
        ucsdet_close(detector);
        return "UNKNOWN";
    }

    // Get detected encoding
    const char* encoding = ucsdet_getName(match, &status);
    int confidence = ucsdet_getConfidence(match, &status);

    // Clean up
    ucsdet_close(detector);

    //LOG_DEBUG_VERBOSE << "Detected encoding: " << encoding << " (Confidence: " << confidence << "%)";
    std::pair<std::string, int> result = std::make_pair(encoding, confidence);
    emailObj.insertAttribute("Encoding", std::make_unique<AttributeBagStringIntPair>(AttributeBagStringIntPair(result)));
    return encoding ? encoding : "UNKNOWN";
}

std::string EmailParser_FSM::convertToUTF8(const std::vector<char>& inputBuffer, const std::string& encoding) {
    UErrorCode status = U_ZERO_ERROR;

    // Open ICU converter for the detected encoding
    UConverter* conv = ucnv_open(encoding.c_str(), &status);
    if (U_FAILURE(status)) {
        throw std::runtime_error("Error: Unable to open ICU converter for " + encoding);
    }

    // Step 1: Calculate required UTF-8 buffer size (nullptr = no conversion)
    status = U_ZERO_ERROR;
    int32_t requiredSize = ucnv_convert("UTF-8", encoding.c_str(), nullptr, 0,
                                        inputBuffer.data(), inputBuffer.size(), &status);
    //LOG_DEBUG_VERBOSE << "Required output buffer size: " << requiredSize;
    if (status != U_BUFFER_OVERFLOW_ERROR) {
        ucnv_close(conv);
        throw std::runtime_error("Error: Failed to calculate buffer size for UTF-8 conversion.");
    }

    // Step 2: Allocate buffer of the correct size
    status = U_ZERO_ERROR;
    std::vector<char> utf8Buffer(requiredSize);

    // Step 3: Perform the actual conversion (with correctly sized target buffer)
    int32_t actualSize = ucnv_convert("UTF-8", encoding.c_str(), utf8Buffer.data(), utf8Buffer.size(),
                                      inputBuffer.data(), inputBuffer.size(), &status);

    // Cleanup
    ucnv_close(conv);

    if (U_FAILURE(status)) {
        throw std::runtime_error("Error: Conversion to UTF-8 failed.");
    }

    return std::string(utf8Buffer.begin(), utf8Buffer.begin() + actualSize);
}

// State handler methods
int EmailParser_FSM::handleNotReading(const std::string& input) {
    if (!input.empty()) { // line not empty
        //LOG_DEBUG_VERBOSE << "Transitioning to Header";
        changeState(ReadingState::Header); // this means it belongs to the header readingstate
        return 0; // say the line is not processed, so the lineparser will hand the exact same line to the next state (header)
    }
    return 1; // if the input was empty, I have processed the whole line and the header does not need it.
}

int EmailParser_FSM::handleHeader(const std::string& input) {
    if (!input.empty()) {
        //LOG_DEBUG_VERBOSE << "Header line: " << input;
        if (input.at(0) == ' ' || input.at(0) == '\t') { // header line is part of the previous line
            headervalstringstream << input;
            return 1;
        }
        if (headerkey.str().empty() && headervalstringstream.str().empty()) {
            //LOG_DEBUG_VERBOSE << "First line of header";
        } else {
            checkIfMIME(headervalstringstream.str());
            emailObj.setHeader(headerkey.str(), headervalstringstream.str());
            headerkey.str(std::string());
            headervalstringstream.str(std::string());
        }

        std::smatch headerkeyMatches;
        if (std::regex_match(input, headerkeyMatches, headerkeyRegex)) {
            headerkey << headerkeyMatches[1].str();
            headervalstringstream << headerkeyMatches[2].str();
        }
        return 1;
    }
    checkIfMIME(headervalstringstream.str());
    if (!isMultipart) {
        //LOG_DEBUG_VERBOSE << "Transitioning to Email Body";
        emailBodyObj = std::make_unique<StandardEmailBody>();
        emailObj.setHeader(headerkey.str(), headervalstringstream.str());
        emailObj.setIsMIMEMultipart(false); // TODO: Check why this is needed. Should create a new emailObj everytime, setting it to false.
        headerkey.str(std::string());
        headervalstringstream.str(std::string());
        changeState(ReadingState::EmailPartBody);
        return 1;
    } else {
        //LOG_DEBUG_VERBOSE << "Transitioning to MIMEMultiPartHeader";
        emailBodyObj = std::make_unique<MIMEMultipartBodies>();
        emailObj.setHeader(headerkey.str(), headervalstringstream.str());
        headerkey.str(std::string());
        headervalstringstream.str(std::string());
        changeState(ReadingState::MIMEMultiPartHeader);
        return 1;
    }
}
void EmailParser_FSM::checkIfMIME(const std::string& headerVal) {
    std::smatch matches;
    if (std::regex_match(headerVal, matches, multipartRegex)) {
        //LOG_DEBUG_VERBOSE << matches.str();
        boundary = matches[1].str();
        //LOG_DEBUG_VERBOSE << "Boundary: " << boundary;
        isMultipart = true;
        emailObj.setIsMIMEMultipart(true);
    }
}


int EmailParser_FSM::handleEmailPartBody(const std::string& input) {
    //LOG_DEBUG_VERBOSE << "Body line: " << input;
    body << input;
    return 1;
}

int EmailParser_FSM::handleMIMEMultiPartHeader(const std::string& input) {
    if (!input.empty()) {
        //LOG_DEBUG_VERBOSE << "MIME Header line: " << input;
        if (input.at(0) == ' ' || input.at(0) == '\t') { // header line is part of the previous line
            headervalstringstream << input;
            return 1;
        }
        if (mimeheadervalstringstream.str().empty() && mimeheaderkey.str().empty()) {
            //LOG_DEBUG_VERBOSE << "First line of mime header";
            mimeheadervalstringstream << input;
            mimeheaderkey << "Boundary";
        } else {
            std::vector<std::string> values;
            values.push_back(mimeheadervalstringstream.str());
            mimeheadermap[mimeheaderkey.str()] = values;
            mimeheadervalstringstream.str(std::string());
            mimeheaderkey.str(std::string());
        }


        std::smatch mimeheaderkeyMatches;
        if (std::regex_match(input, mimeheaderkeyMatches, mimeheaderkeyRegex)) {
            mimeheaderkey << mimeheaderkeyMatches[1].str();
            mimeheadervalstringstream << mimeheaderkeyMatches[2].str();
        }
        return 1;
    } else {
        //LOG_DEBUG_VERBOSE << "Transitioning to MIMEMultiPartBody";
        std::vector<std::string> values;
        values.push_back(mimeheadervalstringstream.str());
        mimeheadermap[mimeheaderkey.str()] = values;
        mimeheadervalstringstream.str(std::string());
        mimeheaderkey.str(std::string());
        changeState(ReadingState::MIMEMultiPartBody);
        return 0;
    }

}

int EmailParser_FSM::handleMIMEMultiPartBody(const std::string& input) {
    if (input == "--" + boundary) {
        //LOG_DEBUG_VERBOSE << "Hit MIME boundary: " << input;
        //LOG_DEBUG_VERBOSE << "Transitioning to MIMEMultiPartHeader";
        dynamic_cast<MIMEMultipartBodies*>(emailBodyObj.get())->addPart(mimeheadermap, mimebody.str());
        mimebody.str(std::string());
        mimeheadermap.clear();
        changeState(ReadingState::MIMEMultiPartHeader);
        return 0;
    }
    if (input == "--" + boundary + "--") {
        //LOG_DEBUG_VERBOSE << "End of MIME parts";
        mimebody << input;
        return 1;
    } else {
        //LOG_DEBUG_VERBOSE << "MIME Body part: " << input;
        mimebody << input;
        return 1;
    }
}

void EmailParser_FSM::flush() {
    if (isMultipart) {
        auto* mimeBody = dynamic_cast<MIMEMultipartBodies*>(emailBodyObj.get());
        if (mimeBody) {
            mimeBody->addPart(mimeheadermap, mimebody.str());
        } else {
            LOG_ERROR << "emailBodyObj is not a MIMEMultipartBodies instance";
            throw std::runtime_error("emailBodyObj is not a StandardEmailBody instance");
        }
    } else {
        auto* standardBody = dynamic_cast<StandardEmailBody*>(emailBodyObj.get());
        if (standardBody) {
            standardBody->setContent(body.str());
        } else {
            LOG_WARNING << "File being loaded is likely not an email.";
        }
    }

    if (currentState == ReadingState::MIMEMultiPartBody || currentState == ReadingState::EmailPartBody) {
        //LOG_DEBUG_VERBOSE << "Flushing";
        changeState(ReadingState::NotReading);
        emailObj.setBody(std::move(emailBodyObj)); // Transfer ownership
        bool isUnique = true;
        emailObj.generateUniqueHash();
        for (const auto& email : *emailList) {
            if (email== emailObj) {
                //LOG_DEBUG_VERBOSE << "Email already exists: " << email.getUniqueHash();
                isUnique = false;
            }
        }
        if (isUnique) {
            //LOG_DEBUG_VERBOSE << "Email doesn't exist: " << emailObj.getUniqueHash();
            emailList->insertEmail(emailObj);
            //LOG_DEBUG_VERBOSE << "Inserting Email: " << emailObj.getUniqueHash();
        }
        resetMemberVars();
    } else {
        LOG_WARNING << "Flushed from wrong state";
        resetMemberVars();
    }
}

void EmailParser_FSM::resetMemberVars() {
    isMultipart = false;
    boundary = "";
    headervalstringstream.str(std::string());
    headerkey.str(std::string());
    body.str(std::string());
    mimeheadervalstringstream.str(std::string());
    mimeheaderkey.str(std::string());
    mimeheadermap.clear();
    mimebody.str(std::string());
    emailBodyObj.reset(); // Reset the unique_ptr to nullptr
    emailObj = Email(); // Reset emailObj for the next email
}

// Helper method to change state
void EmailParser_FSM::changeState(const ReadingState newState) {
    currentState = newState;
}


