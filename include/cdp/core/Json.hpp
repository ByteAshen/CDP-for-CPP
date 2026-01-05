

#pragma once

#include <string>
#include <vector>
#include <map>
#include <variant>
#include <memory>
#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <algorithm>
#include <cctype>
#include <limits>

namespace cdp {

class JsonValue;
using JsonObject = std::map<std::string, JsonValue>;
using JsonArray = std::vector<JsonValue>;
using JsonNull = std::nullptr_t;

class JsonValue {
public:
    using Value = std::variant<
        JsonNull,
        bool,
        int64_t,
        double,
        std::string,
        JsonArray,
        JsonObject
    >;

    JsonValue() : value_(nullptr) {}
    JsonValue(std::nullptr_t) : value_(nullptr) {}
    JsonValue(bool b) : value_(b) {}
    JsonValue(int i) : value_(static_cast<int64_t>(i)) {}
    JsonValue(int64_t i) : value_(i) {}
    
    
    JsonValue(uint64_t u) {
        if (u > static_cast<uint64_t>((std::numeric_limits<int64_t>::max)())) {
            value_ = std::to_string(u);  
        } else {
            value_ = static_cast<int64_t>(u);
        }
    }
    JsonValue(double d) : value_(d) {}
    JsonValue(const char* s) : value_(s ? std::string(s) : std::string()) {}  
    JsonValue(const std::string& s) : value_(s) {}
    JsonValue(std::string&& s) : value_(std::move(s)) {}
    JsonValue(const JsonArray& arr) : value_(arr) {}
    JsonValue(JsonArray&& arr) : value_(std::move(arr)) {}
    JsonValue(const JsonObject& obj) : value_(obj) {}
    JsonValue(JsonObject&& obj) : value_(std::move(obj)) {}

    
    [[nodiscard]] bool isNull() const noexcept { return std::holds_alternative<JsonNull>(value_); }
    [[nodiscard]] bool isBool() const noexcept { return std::holds_alternative<bool>(value_); }
    [[nodiscard]] bool isInt() const noexcept { return std::holds_alternative<int64_t>(value_); }
    [[nodiscard]] bool isDouble() const noexcept { return std::holds_alternative<double>(value_); }
    [[nodiscard]] bool isNumber() const noexcept { return isInt() || isDouble(); }
    [[nodiscard]] bool isString() const noexcept { return std::holds_alternative<std::string>(value_); }
    [[nodiscard]] bool isArray() const noexcept { return std::holds_alternative<JsonArray>(value_); }
    [[nodiscard]] bool isObject() const noexcept { return std::holds_alternative<JsonObject>(value_); }

    
    bool asBool() const { return std::get<bool>(value_); }
    int64_t asInt64() const { return std::get<int64_t>(value_); }
    double asDouble() const { return std::get<double>(value_); }
    const std::string& asString() const { return std::get<std::string>(value_); }
    const JsonArray& asArray() const { return std::get<JsonArray>(value_); }
    JsonArray& asArray() { return std::get<JsonArray>(value_); }
    const JsonObject& asObject() const { return std::get<JsonObject>(value_); }
    JsonObject& asObject() { return std::get<JsonObject>(value_); }

    
    double asNumber() const {
        if (isInt()) return static_cast<double>(std::get<int64_t>(value_));
        return std::get<double>(value_);
    }
    int asInt() const {
        if (isInt()) return static_cast<int>(std::get<int64_t>(value_));
        return static_cast<int>(std::get<double>(value_));
    }

    
    bool getBool(bool def = false) const noexcept { return isBool() ? std::get<bool>(value_) : def; }
    int64_t getInt64(int64_t def = 0) const noexcept {
        if (isInt()) return std::get<int64_t>(value_);
        if (isDouble()) return static_cast<int64_t>(std::get<double>(value_));
        return def;
    }
    double getNumber(double def = 0.0) const noexcept {
        if (isDouble()) return std::get<double>(value_);
        if (isInt()) return static_cast<double>(std::get<int64_t>(value_));
        return def;
    }
    int getInt(int def = 0) const noexcept { return static_cast<int>(getInt64(def)); }
    std::string getString(const std::string& def = "") const { return isString() ? std::get<std::string>(value_) : def; }

    
    uint64_t getUint64(uint64_t def = 0) const noexcept {
        if (isInt()) {
            int64_t v = std::get<int64_t>(value_);
            return v >= 0 ? static_cast<uint64_t>(v) : def;
        }
        if (isString()) {
            try {
                return std::stoull(std::get<std::string>(value_));
            } catch (...) {
                return def;
            }
        }
        if (isDouble()) {
            double v = std::get<double>(value_);
            return v >= 0 ? static_cast<uint64_t>(v) : def;
        }
        return def;
    }

    
    bool tryBool(bool& out) const noexcept {
        if (!isBool()) return false;
        out = std::get<bool>(value_);
        return true;
    }
    bool tryInt64(int64_t& out) const noexcept {
        if (isInt()) { out = std::get<int64_t>(value_); return true; }
        if (isDouble()) { out = static_cast<int64_t>(std::get<double>(value_)); return true; }
        return false;
    }
    bool tryDouble(double& out) const noexcept {
        if (isDouble()) { out = std::get<double>(value_); return true; }
        if (isInt()) { out = static_cast<double>(std::get<int64_t>(value_)); return true; }
        return false;
    }
    bool tryString(std::string& out) const {
        if (!isString()) return false;
        out = std::get<std::string>(value_);
        return true;
    }

    
    JsonValue& operator[](const std::string& key) {
        if (!isObject()) value_ = JsonObject{};
        return std::get<JsonObject>(value_)[key];
    }

    
    const JsonValue& operator[](const std::string& key) const {
        static const JsonValue nullValue; 
        if (!isObject()) return nullValue;
        const auto& obj = std::get<JsonObject>(value_);
        auto it = obj.find(key);
        return it != obj.end() ? it->second : nullValue;
    }

    
    const JsonValue* find(const std::string& key) const {
        if (!isObject()) return nullptr;
        const auto& obj = std::get<JsonObject>(value_);
        auto it = obj.find(key);
        return it != obj.end() ? &it->second : nullptr;
    }

    JsonValue* find(const std::string& key) {
        if (!isObject()) return nullptr;
        auto& obj = std::get<JsonObject>(value_);
        auto it = obj.find(key);
        return it != obj.end() ? &it->second : nullptr;
    }

    
    const JsonValue& at(const std::string& key) const {
        if (!isObject()) throw std::out_of_range("JsonValue::at: not an object");
        const auto& obj = std::get<JsonObject>(value_);
        auto it = obj.find(key);
        if (it == obj.end()) throw std::out_of_range("JsonValue::at: key not found: " + key);
        return it->second;
    }

    JsonValue& at(const std::string& key) {
        if (!isObject()) throw std::out_of_range("JsonValue::at: not an object");
        auto& obj = std::get<JsonObject>(value_);
        auto it = obj.find(key);
        if (it == obj.end()) throw std::out_of_range("JsonValue::at: key not found: " + key);
        return it->second;
    }

    
    JsonValue& operator[](size_t index) {
        return std::get<JsonArray>(value_)[index];
    }

    const JsonValue& operator[](size_t index) const {
        return std::get<JsonArray>(value_)[index];
    }

    
    const JsonValue* get(size_t index) const {
        if (!isArray()) return nullptr;
        const auto& arr = std::get<JsonArray>(value_);
        return index < arr.size() ? &arr[index] : nullptr;
    }

    JsonValue* get(size_t index) {
        if (!isArray()) return nullptr;
        auto& arr = std::get<JsonArray>(value_);
        return index < arr.size() ? &arr[index] : nullptr;
    }

    
    const JsonValue& at(size_t index) const {
        if (!isArray()) throw std::out_of_range("JsonValue::at: not an array");
        const auto& arr = std::get<JsonArray>(value_);
        if (index >= arr.size()) throw std::out_of_range("JsonValue::at: index " + std::to_string(index) + " out of range (size=" + std::to_string(arr.size()) + ")");
        return arr[index];
    }

    JsonValue& at(size_t index) {
        if (!isArray()) throw std::out_of_range("JsonValue::at: not an array");
        auto& arr = std::get<JsonArray>(value_);
        if (index >= arr.size()) throw std::out_of_range("JsonValue::at: index " + std::to_string(index) + " out of range (size=" + std::to_string(arr.size()) + ")");
        return arr[index];
    }

    
    [[nodiscard]] bool contains(const std::string& key) const noexcept {
        if (!isObject()) return false;
        return std::get<JsonObject>(value_).count(key) > 0;
    }

    
    [[nodiscard]] size_t size() const noexcept {
        if (isArray()) return std::get<JsonArray>(value_).size();
        if (isObject()) return std::get<JsonObject>(value_).size();
        return 0;
    }

    [[nodiscard]] bool empty() const noexcept {
        if (isArray()) return std::get<JsonArray>(value_).empty();
        if (isObject()) return std::get<JsonObject>(value_).empty();
        if (isString()) return std::get<std::string>(value_).empty();
        return isNull();
    }

    
    [[nodiscard]] const JsonValue* getPath(const std::string& path) const noexcept {
        const JsonValue* current = this;
        size_t start = 0;
        size_t end = 0;

        while (start < path.length()) {
            
            end = path.find_first_of("/.", start);
            if (end == std::string::npos) end = path.length();

            std::string key = path.substr(start, end - start);
            if (key.empty()) {
                start = end + 1;
                continue;
            }

            
            bool isIndex = !key.empty() && std::all_of(key.begin(), key.end(), ::isdigit);

            if (isIndex && current->isArray()) {
                size_t index = std::stoull(key);
                current = current->get(index);
            } else if (current->isObject()) {
                current = current->find(key);
            } else {
                return nullptr;
            }

            if (!current) return nullptr;
            start = end + 1;
        }
        return current;
    }

    
    [[nodiscard]] int getIntAt(const std::string& path, int defaultValue = 0) const noexcept {
        const JsonValue* val = getPath(path);
        return val ? val->getInt(defaultValue) : defaultValue;
    }

    
    [[nodiscard]] int64_t getInt64At(const std::string& path, int64_t defaultValue = 0) const noexcept {
        const JsonValue* val = getPath(path);
        return val ? val->getInt64(defaultValue) : defaultValue;
    }

    
    [[nodiscard]] uint64_t getUint64At(const std::string& path, uint64_t defaultValue = 0) const noexcept {
        const JsonValue* val = getPath(path);
        return val ? val->getUint64(defaultValue) : defaultValue;
    }

    
    [[nodiscard]] double getDoubleAt(const std::string& path, double defaultValue = 0.0) const noexcept {
        const JsonValue* val = getPath(path);
        return val ? val->getNumber(defaultValue) : defaultValue;
    }

    
    [[nodiscard]] bool getBoolAt(const std::string& path, bool defaultValue = false) const noexcept {
        const JsonValue* val = getPath(path);
        return val ? val->getBool(defaultValue) : defaultValue;
    }

    
    [[nodiscard]] std::string getStringAt(const std::string& path, const std::string& defaultValue = "") const {
        const JsonValue* val = getPath(path);
        return val ? val->getString(defaultValue) : defaultValue;
    }

    
    [[nodiscard]] bool hasPath(const std::string& path) const noexcept {
        return getPath(path) != nullptr;
    }

    
    [[nodiscard]] const JsonValue& operator()(const std::string& path) const {
        static const JsonValue nullValue;
        const JsonValue* val = getPath(path);
        return val ? *val : nullValue;
    }

    
    [[nodiscard]] std::string serialize(bool pretty = false, int indent = 0) const;

    
    [[nodiscard]] static JsonValue parse(const std::string& json);

private:
    Value value_;
};

class JsonParser {
public:
    static JsonValue parse(const std::string& json);

private:
    JsonParser(const std::string& json) : json_(json), pos_(0) {}

    JsonValue parseValue();
    JsonValue parseNull();
    JsonValue parseBool();
    JsonValue parseNumber();
    JsonValue parseString();
    JsonValue parseArray();
    JsonValue parseObject();

    void skipWhitespace();
    char peek() const;
    char consume();
    bool match(const std::string& expected);
    std::string parseStringContent();
    void parseEscapeSequence(std::string& result);
    int parseUnicodeEscape();
    void encodeUtf8(int codepoint, std::string& result);

    const std::string& json_;
    size_t pos_;
};


inline std::string JsonValue::serialize(bool pretty, int indent) const {
    std::ostringstream oss;
    std::string indentStr = pretty ? std::string(indent * 2, ' ') : "";
    std::string childIndent = pretty ? std::string((indent + 1) * 2, ' ') : "";
    std::string newline = pretty ? "\n" : "";
    std::string space = pretty ? " " : "";

    if (isNull()) {
        oss << "null";
    } else if (isBool()) {
        oss << (asBool() ? "true" : "false");
    } else if (isInt()) {
        oss << asInt64();
    } else if (isDouble()) {
        double n = asDouble();
        if (std::isfinite(n) && std::floor(n) == n && std::abs(n) < 1e15) {
            oss << static_cast<int64_t>(n);
        } else {
            oss << std::setprecision(17) << n; 
        }
    } else if (isString()) {
        oss << "\"";
        for (char c : asString()) {
            switch (c) {
                case '"': oss << "\\\""; break;
                case '\\': oss << "\\\\"; break;
                case '\b': oss << "\\b"; break;
                case '\f': oss << "\\f"; break;
                case '\n': oss << "\\n"; break;
                case '\r': oss << "\\r"; break;
                case '\t': oss << "\\t"; break;
                default:
                    if (static_cast<unsigned char>(c) < 0x20) {
                        oss << "\\u" << std::hex << std::setfill('0') << std::setw(4) << static_cast<int>(c);
                    } else {
                        oss << c;
                    }
            }
        }
        oss << "\"";
    } else if (isArray()) {
        const auto& arr = asArray();
        if (arr.empty()) {
            oss << "[]";
        } else {
            oss << "[" << newline;
            for (size_t i = 0; i < arr.size(); ++i) {
                oss << childIndent << arr[i].serialize(pretty, indent + 1);
                if (i + 1 < arr.size()) oss << ",";
                oss << newline;
            }
            oss << indentStr << "]";
        }
    } else if (isObject()) {
        const auto& obj = asObject();
        if (obj.empty()) {
            oss << "{}";
        } else {
            oss << "{" << newline;
            size_t i = 0;
            for (const auto& [key, val] : obj) {
                oss << childIndent << "\"" << key << "\":" << space << val.serialize(pretty, indent + 1);
                if (++i < obj.size()) oss << ",";
                oss << newline;
            }
            oss << indentStr << "}";
        }
    }
    return oss.str();
}

inline JsonValue JsonValue::parse(const std::string& json) {
    return JsonParser::parse(json);
}

} 
