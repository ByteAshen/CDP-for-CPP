

#include "cdp/core/SHA1.hpp"
#include <cstring>
#include <sstream>
#include <iomanip>

namespace cdp {

namespace {
    inline uint32_t rotateLeft(uint32_t value, int bits) {
        return (value << bits) | (value >> (32 - bits));
    }
}

SHA1::SHA1() {
    h_[0] = 0x67452301;
    h_[1] = 0xEFCDAB89;
    h_[2] = 0x98BADCFE;
    h_[3] = 0x10325476;
    h_[4] = 0xC3D2E1F0;
    bufferLen_ = 0;
    totalLen_ = 0;
    finalized_ = false;
}

void SHA1::update(const std::string& data) {
    update(reinterpret_cast<const uint8_t*>(data.data()), data.size());
}

void SHA1::update(const uint8_t* data, size_t len) {
    if (finalized_) return;

    totalLen_ += len;

    
    if (bufferLen_ > 0) {
        size_t toCopy = std::min(64 - bufferLen_, len);
        std::memcpy(buffer_ + bufferLen_, data, toCopy);
        bufferLen_ += toCopy;
        data += toCopy;
        len -= toCopy;

        if (bufferLen_ == 64) {
            processBlock(buffer_);
            bufferLen_ = 0;
        }
    }

    
    while (len >= 64) {
        processBlock(data);
        data += 64;
        len -= 64;
    }

    
    if (len > 0) {
        std::memcpy(buffer_, data, len);
        bufferLen_ = len;
    }
}

void SHA1::processBlock(const uint8_t* block) {
    uint32_t w[80];

    
    for (int i = 0; i < 16; ++i) {
        w[i] = (static_cast<uint32_t>(block[i * 4]) << 24) |
               (static_cast<uint32_t>(block[i * 4 + 1]) << 16) |
               (static_cast<uint32_t>(block[i * 4 + 2]) << 8) |
               static_cast<uint32_t>(block[i * 4 + 3]);
    }

    
    for (int i = 16; i < 80; ++i) {
        w[i] = rotateLeft(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
    }

    uint32_t a = h_[0];
    uint32_t b = h_[1];
    uint32_t c = h_[2];
    uint32_t d = h_[3];
    uint32_t e = h_[4];

    for (int i = 0; i < 80; ++i) {
        uint32_t f, k;

        if (i < 20) {
            f = (b & c) | ((~b) & d);
            k = 0x5A827999;
        } else if (i < 40) {
            f = b ^ c ^ d;
            k = 0x6ED9EBA1;
        } else if (i < 60) {
            f = (b & c) | (b & d) | (c & d);
            k = 0x8F1BBCDC;
        } else {
            f = b ^ c ^ d;
            k = 0xCA62C1D6;
        }

        uint32_t temp = rotateLeft(a, 5) + f + e + k + w[i];
        e = d;
        d = c;
        c = rotateLeft(b, 30);
        b = a;
        a = temp;
    }

    h_[0] += a;
    h_[1] += b;
    h_[2] += c;
    h_[3] += d;
    h_[4] += e;
}

void SHA1::padMessage() {
    uint64_t bitLen = totalLen_ * 8;

    
    buffer_[bufferLen_++] = 0x80;

    
    if (bufferLen_ > 56) {
        while (bufferLen_ < 64) {
            buffer_[bufferLen_++] = 0;
        }
        processBlock(buffer_);
        bufferLen_ = 0;
    }

    
    while (bufferLen_ < 56) {
        buffer_[bufferLen_++] = 0;
    }

    
    buffer_[56] = static_cast<uint8_t>(bitLen >> 56);
    buffer_[57] = static_cast<uint8_t>(bitLen >> 48);
    buffer_[58] = static_cast<uint8_t>(bitLen >> 40);
    buffer_[59] = static_cast<uint8_t>(bitLen >> 32);
    buffer_[60] = static_cast<uint8_t>(bitLen >> 24);
    buffer_[61] = static_cast<uint8_t>(bitLen >> 16);
    buffer_[62] = static_cast<uint8_t>(bitLen >> 8);
    buffer_[63] = static_cast<uint8_t>(bitLen);

    processBlock(buffer_);
}

std::array<uint8_t, 20> SHA1::finalize() {
    if (!finalized_) {
        padMessage();
        finalized_ = true;
    }

    std::array<uint8_t, 20> result;
    for (int i = 0; i < 5; ++i) {
        result[i * 4] = static_cast<uint8_t>(h_[i] >> 24);
        result[i * 4 + 1] = static_cast<uint8_t>(h_[i] >> 16);
        result[i * 4 + 2] = static_cast<uint8_t>(h_[i] >> 8);
        result[i * 4 + 3] = static_cast<uint8_t>(h_[i]);
    }

    return result;
}

std::string SHA1::finalizeHex() {
    auto hash = finalize();
    std::ostringstream oss;
    for (uint8_t byte : hash) {
        oss << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(byte);
    }
    return oss.str();
}

std::array<uint8_t, 20> SHA1::hash(const std::string& data) {
    SHA1 sha;
    sha.update(data);
    return sha.finalize();
}

std::string SHA1::hashHex(const std::string& data) {
    SHA1 sha;
    sha.update(data);
    return sha.finalizeHex();
}

} 
