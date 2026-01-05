

#include "cdp/core/Base64.hpp"

namespace cdp {

constexpr char Base64::ALPHABET[];

std::string Base64::encode(const std::string& input) {
    return encode(reinterpret_cast<const uint8_t*>(input.data()), input.size());
}

std::string Base64::encode(const std::vector<uint8_t>& data) {
    return encode(data.data(), data.size());
}

std::string Base64::encode(const uint8_t* data, size_t len) {
    std::string result;
    result.reserve(((len + 2) / 3) * 4);

    size_t i = 0;
    while (i + 2 < len) {
        uint32_t n = (static_cast<uint32_t>(data[i]) << 16) |
                     (static_cast<uint32_t>(data[i + 1]) << 8) |
                     static_cast<uint32_t>(data[i + 2]);

        result += ALPHABET[(n >> 18) & 0x3F];
        result += ALPHABET[(n >> 12) & 0x3F];
        result += ALPHABET[(n >> 6) & 0x3F];
        result += ALPHABET[n & 0x3F];

        i += 3;
    }

    if (i + 1 == len) {
        uint32_t n = static_cast<uint32_t>(data[i]) << 16;
        result += ALPHABET[(n >> 18) & 0x3F];
        result += ALPHABET[(n >> 12) & 0x3F];
        result += PAD;
        result += PAD;
    } else if (i + 2 == len) {
        uint32_t n = (static_cast<uint32_t>(data[i]) << 16) |
                     (static_cast<uint32_t>(data[i + 1]) << 8);
        result += ALPHABET[(n >> 18) & 0x3F];
        result += ALPHABET[(n >> 12) & 0x3F];
        result += ALPHABET[(n >> 6) & 0x3F];
        result += PAD;
    }

    return result;
}

std::vector<uint8_t> Base64::decode(const std::string& input) {
    std::vector<uint8_t> result;
    result.reserve((input.size() / 4) * 3);

    
    int decodeTable[256];
    for (int i = 0; i < 256; ++i) decodeTable[i] = -1;
    for (int i = 0; i < 64; ++i) {
        decodeTable[static_cast<unsigned char>(ALPHABET[i])] = i;
    }

    size_t i = 0;
    while (i < input.size()) {
        
        while (i < input.size() && (input[i] == ' ' || input[i] == '\n' ||
                                     input[i] == '\r' || input[i] == '\t')) {
            ++i;
        }
        if (i >= input.size()) break;

        
        int vals[4] = {0, 0, 0, 0};
        int padCount = 0;

        for (int j = 0; j < 4 && i < input.size(); ++j) {
            char c = input[i++];
            if (c == PAD) {
                padCount++;
                vals[j] = 0;
            } else {
                int v = decodeTable[static_cast<unsigned char>(c)];
                if (v < 0) {
                    
                    --j;
                    continue;
                }
                vals[j] = v;
            }
        }

        
        uint32_t n = (vals[0] << 18) | (vals[1] << 12) | (vals[2] << 6) | vals[3];

        result.push_back(static_cast<uint8_t>((n >> 16) & 0xFF));
        if (padCount < 2) {
            result.push_back(static_cast<uint8_t>((n >> 8) & 0xFF));
        }
        if (padCount < 1) {
            result.push_back(static_cast<uint8_t>(n & 0xFF));
        }
    }

    return result;
}

std::string Base64::decodeToString(const std::string& input) {
    auto bytes = decode(input);
    return std::string(bytes.begin(), bytes.end());
}

} 
