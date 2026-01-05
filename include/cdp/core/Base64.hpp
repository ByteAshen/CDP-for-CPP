#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace cdp {

class Base64 {
public:
    static std::string encode(const std::string& input);
    static std::string encode(const uint8_t* data, size_t len);
    static std::string encode(const std::vector<uint8_t>& data);

    static std::vector<uint8_t> decode(const std::string& input);
    static std::string decodeToString(const std::string& input);

private:
    static constexpr char ALPHABET[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    static constexpr char PAD = '=';
};

} 
