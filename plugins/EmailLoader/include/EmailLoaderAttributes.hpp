#pragma once
#include "AttributeBagValueInterface.hpp"


class AttributeBagStringIntPair final : public AttributeBagValueInterface {
public:
    explicit AttributeBagStringIntPair(std::pair<std::string, int> val) : value(val) {}

    std::string toString() override {
        return "(" + value.first + ", " + std::to_string(value.second) + ")";
    }

    std::string serializeToString() override {
        return "AttributeBagStringIntPair:" + value.first + ":" + std::to_string(value.second);
    }

    AttributeBagValueInterface* clone() override {
        return new AttributeBagStringIntPair(*this);
    }

    static std::unique_ptr<AttributeBagValueInterface> createFromValue(const std::string& value) {
        size_t sep = value.find(':');
        if (sep == std::string::npos) throw std::runtime_error("Invalid pair format.");
        return std::make_unique<AttributeBagStringIntPair>(
            std::make_pair(value.substr(0, sep), std::stoi(value.substr(sep + 1)))
        );
    }

private:
    std::pair<std::string, int> value;

    struct Register {
        Register() {
            AttributeBagRegistry::getInstance().registerType("AttributeBagStringIntPair", createFromValue);
        }
    };
    static inline Register reg;
};

class AttributeBagStringFloatPairVector final : public AttributeBagValueInterface {
public:
    explicit AttributeBagStringFloatPairVector(std::vector<std::pair<std::string, float>> val) : value(val) {}

    std::string toString() override {
        std::stringstream ss;
        for (const auto& p : value) {
            ss << "(" << p.first << ", " << p.second << ");";
        }
        return ss.str();
    }

    std::string serializeToString() override {
        std::stringstream ss;
        ss << "AttributeBagStringFloatPairVector:";
        for (const auto& p : value) {
            ss << p.first << ":" << p.second << ";";  // Using ';' to separate pairs
        }
        return ss.str();
    }

    AttributeBagValueInterface* clone() override {
        return new AttributeBagStringFloatPairVector(*this);
    }

    static std::unique_ptr<AttributeBagValueInterface> createFromValue(const std::string& value) {
        std::vector<std::pair<std::string, float>> parsedPairs;
        size_t pos = 0, prev = 0;

        while ((pos = value.find(';', prev)) != std::string::npos) {
            std::string pairStr = value.substr(prev, pos - prev);
            size_t sep = pairStr.find(':');
            if (sep == std::string::npos) throw std::runtime_error("Invalid vector pair format.");
            parsedPairs.emplace_back(pairStr.substr(0, sep), std::stof(pairStr.substr(sep + 1)));
            prev = pos + 1;
        }

        return std::make_unique<AttributeBagStringFloatPairVector>(parsedPairs);
    }

private:
    std::vector<std::pair<std::string, float>> value;

    struct Register {
        Register() {
            AttributeBagRegistry::getInstance().registerType("AttributeBagStringFloatPairVector", createFromValue);
        }
    };
    static inline Register reg;
};

class AttributeBagCharVector final : public AttributeBagValueInterface {
public:
    explicit AttributeBagCharVector(std::vector<char> val) : value(val){} ;

    std::string toString() override {
        return std::string(value.begin(), value.end());
    };
    std::string serializeToString() override {
        return "AttributeBagString:" + toString();
    };
    AttributeBagValueInterface* clone() override {
        return new AttributeBagCharVector(*this);
    }
    static std::unique_ptr<AttributeBagValueInterface> createFromValue(const std::string& value) {
        std::vector<char> parsedVal(value.begin(), value.end());
        return std::make_unique<AttributeBagCharVector>(parsedVal);
    }
private:
    std::vector<char> value;

    // Self-registration struct created at runtime, not when instantiated.
    struct Register {
        Register() {
            AttributeBagRegistry::getInstance().registerType("AttributeBagCharVector", createFromValue);
        }
    };
    static inline Register reg; // Static Register instance
};
