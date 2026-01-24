#pragma once
#include <string>
#include <vector>
#include <stdexcept>

namespace crypto {

class Base64 {
public:
    static std::vector<uint8_t> decode(const std::string& encoded) {
        size_t len = encoded.size();
        if (len == 0) return {};

        size_t padding = 0;
        if (len >= 1 && encoded[len - 1] == '=') padding++;
        if (len >= 2 && encoded[len - 2] == '=') padding++;

        size_t output_len = (len * 3) / 4 - padding;
        std::vector<uint8_t> result;
        result.reserve(output_len);

        uint32_t buffer = 0;
        int bits_collected = 0;

        for (char c : encoded) {
            if (c == '=') break;
            if (c == ' ' || c == '\n' || c == '\r' || c == '\t') continue;

            int value = decode_char(c);
            if (value < 0) {
                throw std::runtime_error(std::string("Invalid base64 character: ") + c);
            }

            buffer = (buffer << 6) | value;
            bits_collected += 6;

            if (bits_collected >= 8) {
                bits_collected -= 8;
                result.push_back(static_cast<uint8_t>((buffer >> bits_collected) & 0xFF));
            }
        }

        return result;
    }

    static std::string encode(const uint8_t* data, size_t len) {
        static const char table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

        std::string result;
        result.reserve(((len + 2) / 3) * 4);

        for (size_t i = 0; i < len; i += 3) {
            uint32_t n = static_cast<uint32_t>(data[i]) << 16;
            if (i + 1 < len) n |= static_cast<uint32_t>(data[i + 1]) << 8;
            if (i + 2 < len) n |= static_cast<uint32_t>(data[i + 2]);

            result += table[(n >> 18) & 0x3F];
            result += table[(n >> 12) & 0x3F];
            result += (i + 1 < len) ? table[(n >> 6) & 0x3F] : '=';
            result += (i + 2 < len) ? table[n & 0x3F] : '=';
        }

        return result;
    }

    static std::string encode(const std::vector<uint8_t>& data) {
        return encode(data.data(), data.size());
    }

private:
    static int decode_char(char c) {
        if (c >= 'A' && c <= 'Z') return c - 'A';
        if (c >= 'a' && c <= 'z') return c - 'a' + 26;
        if (c >= '0' && c <= '9') return c - '0' + 52;
        if (c == '+') return 62;
        if (c == '/') return 63;
        return -1;
    }
};

}