#pragma once
#include "sha256.hpp"
#include <vector>

namespace crypto {

class HMAC_SHA256 {
public:
    static constexpr size_t BLOCK_SIZE = SHA256::BLOCK_SIZE;
    static constexpr size_t DIGEST_SIZE = SHA256::DIGEST_SIZE;

    static std::array<uint8_t, DIGEST_SIZE> compute(
        const uint8_t* key, size_t key_len,
        const uint8_t* message, size_t msg_len
    ) {
        std::array<uint8_t, BLOCK_SIZE> k_padded{};

        if (key_len > BLOCK_SIZE) {
            auto hashed_key = SHA256::hash(key, key_len);
            std::memcpy(k_padded.data(), hashed_key.data(), DIGEST_SIZE);
        } else {
            std::memcpy(k_padded.data(), key, key_len);
        }

        std::array<uint8_t, BLOCK_SIZE> i_key_pad{};
        std::array<uint8_t, BLOCK_SIZE> o_key_pad{};

        for (size_t i = 0; i < BLOCK_SIZE; ++i) {
            i_key_pad[i] = k_padded[i] ^ 0x36;
            o_key_pad[i] = k_padded[i] ^ 0x5c;
        }

        SHA256 inner_ctx;
        inner_ctx.update(i_key_pad.data(), BLOCK_SIZE);
        inner_ctx.update(message, msg_len);
        auto inner_hash = inner_ctx.finalize();

        SHA256 outer_ctx;
        outer_ctx.update(o_key_pad.data(), BLOCK_SIZE);
        outer_ctx.update(inner_hash.data(), DIGEST_SIZE);
        return outer_ctx.finalize();
    }

    static std::array<uint8_t, DIGEST_SIZE> compute(
        const std::vector<uint8_t>& key,
        const std::string& message
    ) {
        return compute(
            key.data(), key.size(),
            reinterpret_cast<const uint8_t*>(message.data()), message.size()
        );
    }

    static std::string compute_hex(
        const uint8_t* key, size_t key_len,
        const uint8_t* message, size_t msg_len
    ) {
        auto digest = compute(key, key_len, message, msg_len);
        return to_hex_upper(digest);
    }

    static std::string compute_hex(
        const std::vector<uint8_t>& key,
        const std::string& message
    ) {
        return compute_hex(
            key.data(), key.size(),
            reinterpret_cast<const uint8_t*>(message.data()), message.size()
        );
    }

private:
    static std::string to_hex_upper(const std::array<uint8_t, DIGEST_SIZE>& digest) {
        static const char hex_chars[] = "0123456789ABCDEF";
        std::string result;
        result.reserve(DIGEST_SIZE * 2);
        for (uint8_t byte : digest) {
            result += hex_chars[byte >> 4];
            result += hex_chars[byte & 0x0F];
        }
        return result;
    }
};

}