

#include "cdp/core/Json.hpp"

namespace cdp {

JsonValue JsonParser::parse(const std::string& json) {
    JsonParser parser(json);
    parser.skipWhitespace();
    JsonValue result = parser.parseValue();
    parser.skipWhitespace();
    if (parser.pos_ < parser.json_.size()) {
        throw std::runtime_error("Unexpected characters after JSON value");
    }
    return result;
}

JsonValue JsonParser::parseValue() {
    skipWhitespace();
    if (pos_ >= json_.size()) {
        throw std::runtime_error("Unexpected end of JSON");
    }

    char c = peek();
    if (c == 'n') return parseNull();
    if (c == 't' || c == 'f') return parseBool();
    if (c == '"') return parseString();
    if (c == '[') return parseArray();
    if (c == '{') return parseObject();
    if (c == '-' || (c >= '0' && c <= '9')) return parseNumber();

    throw std::runtime_error(std::string("Unexpected character: ") + c);
}

JsonValue JsonParser::parseNull() {
    if (!match("null")) {
        throw std::runtime_error("Invalid null value");
    }
    return JsonValue(nullptr);
}

JsonValue JsonParser::parseBool() {
    if (match("true")) return JsonValue(true);
    if (match("false")) return JsonValue(false);
    throw std::runtime_error("Invalid boolean value");
}

JsonValue JsonParser::parseNumber() {
    size_t start = pos_;
    bool isFloatingPoint = false;

    if (peek() == '-') consume();

    if (peek() == '0') {
        consume();
    } else if (peek() >= '1' && peek() <= '9') {
        while (pos_ < json_.size() && peek() >= '0' && peek() <= '9') {
            consume();
        }
    } else {
        throw std::runtime_error("Invalid number");
    }

    if (pos_ < json_.size() && peek() == '.') {
        isFloatingPoint = true;
        consume();
        if (pos_ >= json_.size() || peek() < '0' || peek() > '9') {
            throw std::runtime_error("Invalid number: expected digit after decimal point");
        }
        while (pos_ < json_.size() && peek() >= '0' && peek() <= '9') {
            consume();
        }
    }

    if (pos_ < json_.size() && (peek() == 'e' || peek() == 'E')) {
        isFloatingPoint = true;
        consume();
        if (pos_ < json_.size() && (peek() == '+' || peek() == '-')) {
            consume();
        }
        if (pos_ >= json_.size() || peek() < '0' || peek() > '9') {
            throw std::runtime_error("Invalid number: expected digit in exponent");
        }
        while (pos_ < json_.size() && peek() >= '0' && peek() <= '9') {
            consume();
        }
    }

    
    size_t numLen = pos_ - start;

    if (isFloatingPoint) {
        
        return JsonValue(std::stod(json_.substr(start, numLen)));
    }

    
    if (numLen <= 18) {  
        int64_t val = 0;
        bool negative = false;
        size_t i = start;

        if (json_[i] == '-') {
            negative = true;
            i++;
        }

        for (; i < pos_; i++) {
            val = val * 10 + (json_[i] - '0');
        }

        return JsonValue(negative ? -val : val);
    }

    
    std::string numStr = json_.substr(start, numLen);
    try {
        int64_t val = std::stoll(numStr);
        return JsonValue(val);
    } catch (const std::out_of_range&) {
        return JsonValue(std::stod(numStr));
    }
}

JsonValue JsonParser::parseString() {
    if (consume() != '"') {
        throw std::runtime_error("Expected opening quote");
    }

    std::string result = parseStringContent();

    if (consume() != '"') {
        throw std::runtime_error("Expected closing quote");
    }

    return JsonValue(std::move(result));
}

std::string JsonParser::parseStringContent() {
    
    size_t start = pos_;
    while (pos_ < json_.size()) {
        char c = json_[pos_];
        if (c == '"') {
            
            return json_.substr(start, pos_ - start);
        }
        if (c == '\\') break;  
        if (static_cast<unsigned char>(c) < 0x20) {
            throw std::runtime_error("Invalid control character in string");
        }
        pos_++;
    }

    
    std::string result = json_.substr(start, pos_ - start);
    result.reserve(result.size() + 32);  

    while (pos_ < json_.size()) {
        char c = peek();
        if (c == '"') break;
        if (c == '\\') {
            consume();
            parseEscapeSequence(result);
        } else if (static_cast<unsigned char>(c) < 0x20) {
            throw std::runtime_error("Invalid control character in string");
        } else {
            result += consume();
        }
    }

    return result;
}


int JsonParser::parseUnicodeEscape() {
    if (pos_ + 4 > json_.size()) {
        throw std::runtime_error("Invalid unicode escape: incomplete");
    }
    
    int result = 0;
    for (int i = 0; i < 4; i++) {
        char c = json_[pos_ + i];
        int digit;
        if (c >= '0' && c <= '9') digit = c - '0';
        else if (c >= 'a' && c <= 'f') digit = 10 + (c - 'a');
        else if (c >= 'A' && c <= 'F') digit = 10 + (c - 'A');
        else throw std::runtime_error("Invalid unicode escape: non-hex character");
        result = (result << 4) | digit;
    }
    pos_ += 4;
    return result;
}


void JsonParser::encodeUtf8(int codepoint, std::string& result) {
    if (codepoint < 0x80) {
        result += static_cast<char>(codepoint);
    } else if (codepoint < 0x800) {
        result += static_cast<char>(0xC0 | (codepoint >> 6));
        result += static_cast<char>(0x80 | (codepoint & 0x3F));
    } else if (codepoint < 0x10000) {
        result += static_cast<char>(0xE0 | (codepoint >> 12));
        result += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
        result += static_cast<char>(0x80 | (codepoint & 0x3F));
    } else if (codepoint <= 0x10FFFF) {
        result += static_cast<char>(0xF0 | (codepoint >> 18));
        result += static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F));
        result += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
        result += static_cast<char>(0x80 | (codepoint & 0x3F));
    } else {
        throw std::runtime_error("Invalid unicode codepoint");
    }
}

void JsonParser::parseEscapeSequence(std::string& result) {
    if (pos_ >= json_.size()) {
        throw std::runtime_error("Unexpected end of string escape");
    }

    char c = consume();
    switch (c) {
        case '"': result += '"'; break;
        case '\\': result += '\\'; break;
        case '/': result += '/'; break;
        case 'b': result += '\b'; break;
        case 'f': result += '\f'; break;
        case 'n': result += '\n'; break;
        case 'r': result += '\r'; break;
        case 't': result += '\t'; break;
        case 'u': {
            int codepoint = parseUnicodeEscape();

            
            if (codepoint >= 0xD800 && codepoint <= 0xDBFF) {
                
                if (pos_ + 2 > json_.size() || json_[pos_] != '\\' || json_[pos_ + 1] != 'u') {
                    throw std::runtime_error("Invalid unicode: high surrogate without low surrogate");
                }
                pos_ += 2; 
                int lowSurrogate = parseUnicodeEscape();
                if (lowSurrogate < 0xDC00 || lowSurrogate > 0xDFFF) {
                    throw std::runtime_error("Invalid unicode: expected low surrogate");
                }
                codepoint = 0x10000 + ((codepoint - 0xD800) << 10) + (lowSurrogate - 0xDC00);
            } else if (codepoint >= 0xDC00 && codepoint <= 0xDFFF) {
                throw std::runtime_error("Invalid unicode: unexpected low surrogate");
            }

            encodeUtf8(codepoint, result);
            break;
        }
        default:
            throw std::runtime_error(std::string("Invalid escape character: ") + c);
    }
}

JsonValue JsonParser::parseArray() {
    if (consume() != '[') {
        throw std::runtime_error("Expected opening bracket");
    }

    JsonArray result;
    skipWhitespace();

    if (peek() == ']') {
        consume();
        return JsonValue(std::move(result));
    }

    while (true) {
        result.push_back(parseValue());
        skipWhitespace();

        char c = consume();
        if (c == ']') break;
        if (c != ',') {
            throw std::runtime_error("Expected comma or closing bracket");
        }
        skipWhitespace();
    }

    return JsonValue(std::move(result));
}

JsonValue JsonParser::parseObject() {
    if (consume() != '{') {
        throw std::runtime_error("Expected opening brace");
    }

    JsonObject result;
    skipWhitespace();

    if (peek() == '}') {
        consume();
        return JsonValue(std::move(result));
    }

    while (true) {
        skipWhitespace();
        if (peek() != '"') {
            throw std::runtime_error("Expected property name");
        }

        consume(); 
        std::string key = parseStringContent();
        if (consume() != '"') {
            throw std::runtime_error("Expected closing quote for property name");
        }

        skipWhitespace();
        if (consume() != ':') {
            throw std::runtime_error("Expected colon after property name");
        }

        result[key] = parseValue();
        skipWhitespace();

        char c = consume();
        if (c == '}') break;
        if (c != ',') {
            throw std::runtime_error("Expected comma or closing brace");
        }
    }

    return JsonValue(std::move(result));
}

void JsonParser::skipWhitespace() {
    while (pos_ < json_.size()) {
        char c = json_[pos_];
        if (c != ' ' && c != '\t' && c != '\n' && c != '\r') break;
        ++pos_;
    }
}

char JsonParser::peek() const {
    return pos_ < json_.size() ? json_[pos_] : '\0';
}

char JsonParser::consume() {
    return pos_ < json_.size() ? json_[pos_++] : '\0';
}

bool JsonParser::match(const std::string& expected) {
    size_t len = expected.size();
    if (pos_ + len > json_.size()) return false;
    
    if (json_.compare(pos_, len, expected) != 0) return false;
    pos_ += len;
    return true;
}

} 
