#pragma once
#include "sha256.hpp"
#include "base64.hpp"
#include "platform.hpp"
#include <string>

namespace chrome {

inline std::string generate_extension_id(const std::string& extension_path) {
    std::string normalized = platform::normalize_path(extension_path);
    std::vector<uint8_t> path_bytes = platform::path_to_bytes(normalized);

    auto hash = crypto::SHA256::hash(path_bytes.data(), path_bytes.size());
    std::string hash_hex = crypto::SHA256::to_hex(hash);

    std::string id;
    id.reserve(32);
    for (int i = 0; i < 32; ++i) {
        char c = hash_hex[i];
        int val;
        if (c >= '0' && c <= '9') {
            val = c - '0';
        } else {
            val = c - 'a' + 10;
        }
        id += static_cast<char>('a' + val);
    }
    return id;
}

inline std::string generate_extension_id_from_key(const std::string& key) {
    std::vector<uint8_t> key_bytes = crypto::Base64::decode(key);

    auto hash = crypto::SHA256::hash(key_bytes.data(), key_bytes.size());
    std::string hash_hex = crypto::SHA256::to_hex(hash);

    std::string id;
    id.reserve(32);
    for (int i = 0; i < 32; ++i) {
        char c = hash_hex[i];
        int val;
        if (c >= '0' && c <= '9') {
            val = c - '0';
        } else {
            val = c - 'a' + 10;
        }
        id += static_cast<char>('a' + val);
    }
    return id;
}

}