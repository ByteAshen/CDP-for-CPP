#pragma once
#include <string>
#include <vector>
#include <map>
#include <variant>
#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace json {

class Value;

using Object = std::map<std::string, Value>;
using Array = std::vector<Value>;

class Value {
public:
    using Variant = std::variant<
        std::nullptr_t,
        bool,
        int64_t,
        double,
        std::string,
        Array,
        Object
    >;

    Value() : m_data(nullptr) {}
    Value(std::nullptr_t) : m_data(nullptr) {}
    Value(bool v) : m_data(v) {}
    Value(int v) : m_data(static_cast<int64_t>(v)) {}
    Value(int64_t v) : m_data(v) {}
    Value(double v) : m_data(v) {}
    Value(const char* v) : m_data(std::string(v)) {}
    Value(const std::string& v) : m_data(v) {}
    Value(std::string&& v) : m_data(std::move(v)) {}
    Value(const Array& v) : m_data(v) {}
    Value(Array&& v) : m_data(std::move(v)) {}
    Value(const Object& v) : m_data(v) {}
    Value(Object&& v) : m_data(std::move(v)) {}

    bool is_null() const { return std::holds_alternative<std::nullptr_t>(m_data); }
    bool is_bool() const { return std::holds_alternative<bool>(m_data); }
    bool is_int() const { return std::holds_alternative<int64_t>(m_data); }
    bool is_double() const { return std::holds_alternative<double>(m_data); }
    bool is_string() const { return std::holds_alternative<std::string>(m_data); }
    bool is_array() const { return std::holds_alternative<Array>(m_data); }
    bool is_object() const { return std::holds_alternative<Object>(m_data); }

    bool as_bool() const { return std::get<bool>(m_data); }
    int64_t as_int() const { return std::get<int64_t>(m_data); }
    double as_double() const { return std::get<double>(m_data); }
    const std::string& as_string() const { return std::get<std::string>(m_data); }
    std::string& as_string() { return std::get<std::string>(m_data); }
    const Array& as_array() const { return std::get<Array>(m_data); }
    Array& as_array() { return std::get<Array>(m_data); }
    const Object& as_object() const { return std::get<Object>(m_data); }
    Object& as_object() { return std::get<Object>(m_data); }

    Value& operator[](const std::string& key) {
        if (!is_object()) {
            m_data = Object{};
        }
        return std::get<Object>(m_data)[key];
    }

    const Value& operator[](const std::string& key) const {
        return std::get<Object>(m_data).at(key);
    }

    Value& operator[](size_t index) {
        return std::get<Array>(m_data)[index];
    }

    const Value& operator[](size_t index) const {
        return std::get<Array>(m_data)[index];
    }

    bool contains(const std::string& key) const {
        if (!is_object()) return false;
        const auto& obj = std::get<Object>(m_data);
        return obj.find(key) != obj.end();
    }

    template<typename T>
    T get(const std::string& key, const T& default_val) const {
        if (!is_object()) return default_val;
        const auto& obj = std::get<Object>(m_data);
        auto it = obj.find(key);
        if (it == obj.end()) return default_val;
        if constexpr (std::is_same_v<T, std::string>) {
            return it->second.is_string() ? it->second.as_string() : default_val;
        } else if constexpr (std::is_same_v<T, bool>) {
            return it->second.is_bool() ? it->second.as_bool() : default_val;
        } else if constexpr (std::is_integral_v<T>) {
            return it->second.is_int() ? static_cast<T>(it->second.as_int()) : default_val;
        }
        return default_val;
    }

    const Variant& data() const { return m_data; }

private:
    Variant m_data;
};

class Serializer {
public:
    static std::string serialize(const Value& value, bool escape_lt = false) {
        std::string result;
        serialize_value(value, result, escape_lt);
        return result;
    }

private:
    static void serialize_value(const Value& value, std::string& out, bool escape_lt) {
        if (value.is_null()) {
            out += "null";
        } else if (value.is_bool()) {
            out += value.as_bool() ? "true" : "false";
        } else if (value.is_int()) {
            out += std::to_string(value.as_int());
        } else if (value.is_double()) {
            std::ostringstream ss;
            ss << std::setprecision(17) << value.as_double();
            out += ss.str();
        } else if (value.is_string()) {
            serialize_string(value.as_string(), out, escape_lt);
        } else if (value.is_array()) {
            serialize_array(value.as_array(), out, escape_lt);
        } else if (value.is_object()) {
            serialize_object(value.as_object(), out, escape_lt);
        }
    }

    static void serialize_string(const std::string& str, std::string& out, bool escape_lt) {
        out += '"';
        for (char c : str) {
            switch (c) {
                case '"': out += "\\\""; break;
                case '\\': out += "\\\\"; break;
                case '\b': out += "\\b"; break;
                case '\f': out += "\\f"; break;
                case '\n': out += "\\n"; break;
                case '\r': out += "\\r"; break;
                case '\t': out += "\\t"; break;
                case '<':
                    if (escape_lt) {
                        out += "\\u003C";
                    } else {
                        out += c;
                    }
                    break;
                default:
                    if (static_cast<unsigned char>(c) < 0x20) {
                        out += "\\u00";
                        out += "0123456789ABCDEF"[(c >> 4) & 0xF];
                        out += "0123456789ABCDEF"[c & 0xF];
                    } else {
                        out += c;
                    }
                    break;
            }
        }
        out += '"';
    }

    static void serialize_array(const Array& arr, std::string& out, bool escape_lt) {
        out += '[';
        bool first = true;
        for (const auto& elem : arr) {
            if (!first) out += ',';
            first = false;
            serialize_value(elem, out, escape_lt);
        }
        out += ']';
    }

    static void serialize_object(const Object& obj, std::string& out, bool escape_lt) {
        out += '{';
        bool first = true;
        for (const auto& [key, value] : obj) {
            if (!first) out += ',';
            first = false;
            serialize_string(key, out, escape_lt);
            out += ':';
            serialize_value(value, out, escape_lt);
        }
        out += '}';
    }
};

class Parser {
public:
    static Value parse(const std::string& json) {
        Parser p(json);
        return p.parse_value();
    }

private:
    Parser(const std::string& json) : m_json(json), m_pos(0) {}

    Value parse_value() {
        skip_whitespace();
        if (m_pos >= m_json.size()) {
            throw std::runtime_error("Unexpected end of JSON");
        }

        char c = m_json[m_pos];
        if (c == 'n') return parse_null();
        if (c == 't' || c == 'f') return parse_bool();
        if (c == '"') return parse_string();
        if (c == '[') return parse_array();
        if (c == '{') return parse_object();
        if (c == '-' || (c >= '0' && c <= '9')) return parse_number();

        throw std::runtime_error(std::string("Unexpected character: ") + c);
    }

    Value parse_null() {
        expect("null");
        return Value(nullptr);
    }

    Value parse_bool() {
        if (m_json[m_pos] == 't') {
            expect("true");
            return Value(true);
        } else {
            expect("false");
            return Value(false);
        }
    }

    Value parse_number() {
        size_t start = m_pos;
        bool is_float = false;

        if (m_json[m_pos] == '-') m_pos++;

        while (m_pos < m_json.size() && m_json[m_pos] >= '0' && m_json[m_pos] <= '9') {
            m_pos++;
        }

        if (m_pos < m_json.size() && m_json[m_pos] == '.') {
            is_float = true;
            m_pos++;
            while (m_pos < m_json.size() && m_json[m_pos] >= '0' && m_json[m_pos] <= '9') {
                m_pos++;
            }
        }

        if (m_pos < m_json.size() && (m_json[m_pos] == 'e' || m_json[m_pos] == 'E')) {
            is_float = true;
            m_pos++;
            if (m_pos < m_json.size() && (m_json[m_pos] == '+' || m_json[m_pos] == '-')) {
                m_pos++;
            }
            while (m_pos < m_json.size() && m_json[m_pos] >= '0' && m_json[m_pos] <= '9') {
                m_pos++;
            }
        }

        std::string num_str = m_json.substr(start, m_pos - start);
        if (is_float) {
            return Value(std::stod(num_str));
        } else {
            return Value(static_cast<int64_t>(std::stoll(num_str)));
        }
    }

    Value parse_string() {
        m_pos++;
        std::string result;

        while (m_pos < m_json.size() && m_json[m_pos] != '"') {
            if (m_json[m_pos] == '\\') {
                m_pos++;
                if (m_pos >= m_json.size()) {
                    throw std::runtime_error("Unexpected end of string");
                }
                switch (m_json[m_pos]) {
                    case '"': result += '"'; break;
                    case '\\': result += '\\'; break;
                    case '/': result += '/'; break;
                    case 'b': result += '\b'; break;
                    case 'f': result += '\f'; break;
                    case 'n': result += '\n'; break;
                    case 'r': result += '\r'; break;
                    case 't': result += '\t'; break;
                    case 'u': {
                        m_pos++;
                        if (m_pos + 4 > m_json.size()) {
                            throw std::runtime_error("Invalid unicode escape");
                        }
                        std::string hex = m_json.substr(m_pos, 4);
                        uint32_t codepoint = std::stoul(hex, nullptr, 16);
                        if (codepoint < 0x80) {
                            result += static_cast<char>(codepoint);
                        } else if (codepoint < 0x800) {
                            result += static_cast<char>(0xC0 | (codepoint >> 6));
                            result += static_cast<char>(0x80 | (codepoint & 0x3F));
                        } else {
                            result += static_cast<char>(0xE0 | (codepoint >> 12));
                            result += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
                            result += static_cast<char>(0x80 | (codepoint & 0x3F));
                        }
                        m_pos += 3;
                        break;
                    }
                    default:
                        throw std::runtime_error(std::string("Invalid escape sequence: \\") + m_json[m_pos]);
                }
            } else {
                result += m_json[m_pos];
            }
            m_pos++;
        }

        if (m_pos >= m_json.size()) {
            throw std::runtime_error("Unterminated string");
        }
        m_pos++;
        return Value(std::move(result));
    }

    Value parse_array() {
        m_pos++;
        Array result;

        skip_whitespace();
        if (m_pos < m_json.size() && m_json[m_pos] == ']') {
            m_pos++;
            return Value(std::move(result));
        }

        while (true) {
            result.push_back(parse_value());
            skip_whitespace();

            if (m_pos >= m_json.size()) {
                throw std::runtime_error("Unterminated array");
            }

            if (m_json[m_pos] == ']') {
                m_pos++;
                break;
            }

            if (m_json[m_pos] != ',') {
                throw std::runtime_error("Expected ',' in array");
            }
            m_pos++;
        }

        return Value(std::move(result));
    }

    Value parse_object() {
        m_pos++;
        Object result;

        skip_whitespace();
        if (m_pos < m_json.size() && m_json[m_pos] == '}') {
            m_pos++;
            return Value(std::move(result));
        }

        while (true) {
            skip_whitespace();
            if (m_pos >= m_json.size() || m_json[m_pos] != '"') {
                throw std::runtime_error("Expected string key in object");
            }

            Value key_val = parse_string();
            std::string key = key_val.as_string();

            skip_whitespace();
            if (m_pos >= m_json.size() || m_json[m_pos] != ':') {
                throw std::runtime_error("Expected ':' after key");
            }
            m_pos++;

            Value value = parse_value();
            result[key] = std::move(value);

            skip_whitespace();
            if (m_pos >= m_json.size()) {
                throw std::runtime_error("Unterminated object");
            }

            if (m_json[m_pos] == '}') {
                m_pos++;
                break;
            }

            if (m_json[m_pos] != ',') {
                throw std::runtime_error("Expected ',' in object");
            }
            m_pos++;
        }

        return Value(std::move(result));
    }

    void skip_whitespace() {
        while (m_pos < m_json.size() &&
               (m_json[m_pos] == ' ' || m_json[m_pos] == '\t' ||
                m_json[m_pos] == '\n' || m_json[m_pos] == '\r')) {
            m_pos++;
        }
    }

    void expect(const char* str) {
        size_t len = std::strlen(str);
        if (m_pos + len > m_json.size() || m_json.substr(m_pos, len) != str) {
            throw std::runtime_error(std::string("Expected: ") + str);
        }
        m_pos += len;
    }

    const std::string& m_json;
    size_t m_pos;
};

}