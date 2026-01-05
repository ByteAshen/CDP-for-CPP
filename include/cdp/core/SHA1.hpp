#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <array>

namespace cdp {

class SHA1 {
public:
    SHA1();

    void update(const uint8_t* data, size_t len);
    void update(const std::string& data);

    std::array<uint8_t, 20> finalize();
    std::string finalizeHex();

    static std::array<uint8_t, 20> hash(const std::string& data);
    static std::string hashHex(const std::string& data);

private:
    void processBlock(const uint8_t* block);
    void padMessage();

    uint32_t h_[5];
    uint8_t buffer_[64];
    size_t bufferLen_;
    uint64_t totalLen_;
    bool finalized_;
};

} 
