#pragma once
#include <string>
#include <cstdint>
#include <vector>
#include <functional>
#include <unordered_map>

#include "Logger.hpp"



// Deserialize helper function
inline std::pair<std::string, std::string> splitSerializedString(const std::string& serialized) {
    size_t pos = serialized.find(':');
    if (pos == std::string::npos) {
        throw std::runtime_error("Invalid serialized format: Missing ':' delimiter.");
    }
    return { serialized.substr(0, pos), serialized.substr(pos + 1) };
}

class AttributeBagValueInterface {
public:
    virtual ~AttributeBagValueInterface() = default;
    virtual std::string toString() = 0;
    // virtual somethin toByte() = 0;
    virtual std::string serializeToString() = 0;
    virtual AttributeBagValueInterface* clone() = 0;
};

class AttributeBagRegistry {
public:
    using FactoryFunc = std::function<std::unique_ptr<AttributeBagValueInterface>(const std::string&)>;

    static AttributeBagRegistry& getInstance() {
        static AttributeBagRegistry instance;
        return instance;
    }

    static std::unique_ptr<AttributeBagValueInterface> deserializeAttribute(const std::string& serialized) {
        auto [type, value] = splitSerializedString(serialized);
        return getInstance().create(type, value);
    }

    void registerType(const std::string& typeName, FactoryFunc factory) {
        registry_[typeName] = std::move(factory);
    }

    std::unique_ptr<AttributeBagValueInterface> create(const std::string& typeName, const std::string& serializedData) {
        if (registry_.contains(typeName)) {
            return registry_[typeName](serializedData);
        }
        LOG_ERROR << "Trying to deserialize unknown AttributeBagValueInterface implementation: " << typeName;
        return nullptr; // Type not found
    }

private:
    std::unordered_map<std::string, FactoryFunc> registry_;
    AttributeBagRegistry() = default;
};

class AttributeBagString final : public AttributeBagValueInterface {
public:
    explicit AttributeBagString(std::string val) : value(val){} ;

    std::string toString() override {
        return value;
    };
    std::string serializeToString() override {
        return "AttributeBagString:" + value;
    };
     AttributeBagValueInterface* clone() override {
        return new AttributeBagString(*this);
    }
    static std::unique_ptr<AttributeBagValueInterface> createFromValue(const std::string& value) {
         return std::make_unique<AttributeBagString>(value);
    }
private:
    std::string value;

    // Self-registration struct created at runtime, not when instantiated.
    struct Register {
        Register() {
            AttributeBagRegistry::getInstance().registerType("AttributeBagString", createFromValue);
        }
    };
    static inline Register reg; // Static Register instance
};

class AttributeBagBoolean final : public AttributeBagValueInterface {
public:
    explicit AttributeBagBoolean(bool val) : value(val){}
    std::string toString() override {
        return std::to_string(value);
    };
    std::string serializeToString() override {
        return "AttributeBagBoolean:" + std::to_string(value);
    }
     AttributeBagValueInterface* clone() override {
        return new AttributeBagBoolean(*this);
    }
    static std::unique_ptr<AttributeBagValueInterface> createFromValue(const std::string& value) {
        return std::make_unique<AttributeBagBoolean>(value == "1");
    }
private:
    bool value;
    struct Register {
        Register() {
            AttributeBagRegistry::getInstance().registerType("AttributeBagBoolean", createFromValue);
        }
    };
    static inline Register reg;
};

class AttributeBagInteger final : public AttributeBagValueInterface {
public:
    explicit AttributeBagInteger(int val) : value(val){}
    std::string toString() override {
        return std::to_string(value);
    }
    std::string serializeToString() override {
        return "AttributeBagInteger:" + std::to_string(value);
    }
     AttributeBagValueInterface* clone() override {
        return new AttributeBagInteger(*this);
    }
    static std::unique_ptr<AttributeBagValueInterface> createFromValue(const std::string& value) {
        return std::make_unique<AttributeBagInteger>(std::stoi(value));
    }
private:
    int value;

    struct Register {
        Register() {
            AttributeBagRegistry::getInstance().registerType("AttributeBagInteger", createFromValue);
        }
    };
    static inline Register reg;
};

class AttributeBagDouble final : public AttributeBagValueInterface {
public:
    explicit AttributeBagDouble(double val) : value(val){}
    std::string toString() override {
        return std::to_string(value);
    }
    std::string serializeToString() override {
        return "AttributeBagDouble:" + std::to_string(value);
    }
    AttributeBagValueInterface* clone() override {
        return new AttributeBagDouble(*this);
    }
    static std::unique_ptr<AttributeBagValueInterface> createFromValue(const std::string& value) {
        return std::make_unique<AttributeBagDouble>(std::stod(value));
    }
private:
    double value;
    struct Register {
        Register() {
            AttributeBagRegistry::getInstance().registerType("AttributeBagDouble",createFromValue);
        }
    };
    static inline Register reg;
};

class AttributeBagBinary final : public AttributeBagValueInterface {
public:
    explicit AttributeBagBinary(std::vector<std::uint8_t> val) : value(val){};
    std::string toString() override {
        return reinterpret_cast<const char*>(value.data());
    }
    std::string serializeToString() override {
        return "AttributeBagBinary:" + toString();
    }
    AttributeBagValueInterface* clone() override {
        return new AttributeBagBinary(*this);
    }
    static std::unique_ptr<AttributeBagValueInterface> createFromValue(const std::string& value) {
        std::vector<uint8_t> binaryData(value.begin(), value.end());  // Convert string to binary vector
        return std::make_unique<AttributeBagBinary>(binaryData);
    }
private:
    std::vector<std::uint8_t> value;
    struct Register {
        Register() {
            AttributeBagRegistry::getInstance().registerType("AttributeBagBinary", createFromValue);
        }
    };
    static inline Register reg;
};
