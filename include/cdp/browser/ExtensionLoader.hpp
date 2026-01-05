#pragma once


#include <string>
#include <vector>
#include <map>
#include <set>
#include <array>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <chrono>
#include <algorithm>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <sddl.h>
#pragma comment(lib, "advapi32.lib")
#endif

#include "../core/Json.hpp"
#include "../core/Base64.hpp"

namespace cdp {
namespace extension {


class SHA256 {
public:
    static constexpr size_t BLOCK_SIZE = 64;
    static constexpr size_t DIGEST_SIZE = 32;

    SHA256() { reset(); }

    void reset() {
        m_state[0] = 0x6a09e667;
        m_state[1] = 0xbb67ae85;
        m_state[2] = 0x3c6ef372;
        m_state[3] = 0xa54ff53a;
        m_state[4] = 0x510e527f;
        m_state[5] = 0x9b05688c;
        m_state[6] = 0x1f83d9ab;
        m_state[7] = 0x5be0cd19;
        m_total_len = 0;
        m_buffer_len = 0;
    }

    void update(const uint8_t* data, size_t len) {
        m_total_len += len;

        if (m_buffer_len > 0) {
            size_t to_copy = (std::min)(len, BLOCK_SIZE - m_buffer_len);
            std::memcpy(m_buffer + m_buffer_len, data, to_copy);
            m_buffer_len += to_copy;
            data += to_copy;
            len -= to_copy;

            if (m_buffer_len == BLOCK_SIZE) {
                process_block(m_buffer);
                m_buffer_len = 0;
            }
        }

        while (len >= BLOCK_SIZE) {
            process_block(data);
            data += BLOCK_SIZE;
            len -= BLOCK_SIZE;
        }

        if (len > 0) {
            std::memcpy(m_buffer, data, len);
            m_buffer_len = len;
        }
    }

    void update(const std::string& data) {
        update(reinterpret_cast<const uint8_t*>(data.data()), data.size());
    }

    std::array<uint8_t, DIGEST_SIZE> finalize() {
        uint64_t total_bits = m_total_len * 8;
        m_buffer[m_buffer_len++] = 0x80;

        if (m_buffer_len > 56) {
            std::memset(m_buffer + m_buffer_len, 0, BLOCK_SIZE - m_buffer_len);
            process_block(m_buffer);
            m_buffer_len = 0;
        }

        std::memset(m_buffer + m_buffer_len, 0, 56 - m_buffer_len);

        for (int i = 0; i < 8; ++i) {
            m_buffer[56 + i] = static_cast<uint8_t>(total_bits >> (56 - i * 8));
        }
        process_block(m_buffer);

        std::array<uint8_t, DIGEST_SIZE> digest;
        for (int i = 0; i < 8; ++i) {
            digest[i * 4 + 0] = static_cast<uint8_t>(m_state[i] >> 24);
            digest[i * 4 + 1] = static_cast<uint8_t>(m_state[i] >> 16);
            digest[i * 4 + 2] = static_cast<uint8_t>(m_state[i] >> 8);
            digest[i * 4 + 3] = static_cast<uint8_t>(m_state[i]);
        }
        return digest;
    }

    static std::array<uint8_t, DIGEST_SIZE> hash(const uint8_t* data, size_t len) {
        SHA256 ctx;
        ctx.update(data, len);
        return ctx.finalize();
    }

    static std::array<uint8_t, DIGEST_SIZE> hash(const std::string& data) {
        return hash(reinterpret_cast<const uint8_t*>(data.data()), data.size());
    }

    static std::string to_hex(const std::array<uint8_t, DIGEST_SIZE>& digest) {
        static const char hex_chars[] = "0123456789abcdef";
        std::string result;
        result.reserve(DIGEST_SIZE * 2);
        for (uint8_t byte : digest) {
            result += hex_chars[byte >> 4];
            result += hex_chars[byte & 0x0F];
        }
        return result;
    }

private:
    void process_block(const uint8_t* block) {
        static constexpr uint32_t K[64] = {
            0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
            0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
            0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
            0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
            0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
            0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
            0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
            0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
            0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
            0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
            0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
            0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
            0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
            0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
            0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
            0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
        };

        uint32_t W[64];
        for (int i = 0; i < 16; ++i) {
            W[i] = (static_cast<uint32_t>(block[i * 4 + 0]) << 24) |
                   (static_cast<uint32_t>(block[i * 4 + 1]) << 16) |
                   (static_cast<uint32_t>(block[i * 4 + 2]) << 8) |
                   (static_cast<uint32_t>(block[i * 4 + 3]));
        }
        for (int i = 16; i < 64; ++i) {
            uint32_t s0 = rotr(W[i - 15], 7) ^ rotr(W[i - 15], 18) ^ (W[i - 15] >> 3);
            uint32_t s1 = rotr(W[i - 2], 17) ^ rotr(W[i - 2], 19) ^ (W[i - 2] >> 10);
            W[i] = W[i - 16] + s0 + W[i - 7] + s1;
        }

        uint32_t a = m_state[0], b = m_state[1], c = m_state[2], d = m_state[3];
        uint32_t e = m_state[4], f = m_state[5], g = m_state[6], h = m_state[7];

        for (int i = 0; i < 64; ++i) {
            uint32_t S1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
            uint32_t ch = (e & f) ^ (~e & g);
            uint32_t temp1 = h + S1 + ch + K[i] + W[i];
            uint32_t S0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
            uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
            uint32_t temp2 = S0 + maj;

            h = g; g = f; f = e; e = d + temp1;
            d = c; c = b; b = a; a = temp1 + temp2;
        }

        m_state[0] += a; m_state[1] += b; m_state[2] += c; m_state[3] += d;
        m_state[4] += e; m_state[5] += f; m_state[6] += g; m_state[7] += h;
    }

    static constexpr uint32_t rotr(uint32_t x, int n) {
        return (x >> n) | (x << (32 - n));
    }

    uint32_t m_state[8];
    uint8_t m_buffer[BLOCK_SIZE];
    size_t m_buffer_len;
    uint64_t m_total_len;
};


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
        const std::vector<uint8_t>& key,
        const std::string& message
    ) {
        auto digest = compute(key, message);
        return to_hex_upper(digest);
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


namespace platform {


inline std::string get_sid() {
#ifdef _WIN32
    HANDLE token = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) {
        return "";
    }

    DWORD size = 0;
    GetTokenInformation(token, TokenUser, nullptr, 0, &size);
    if (size == 0) {
        CloseHandle(token);
        return "";
    }

    std::vector<uint8_t> buffer(size);
    TOKEN_USER* token_user = reinterpret_cast<TOKEN_USER*>(buffer.data());

    if (!GetTokenInformation(token, TokenUser, token_user, size, &size)) {
        CloseHandle(token);
        return "";
    }

    LPSTR sid_string = nullptr;
    if (!ConvertSidToStringSidA(token_user->User.Sid, &sid_string)) {
        CloseHandle(token);
        return "";
    }

    std::string sid(sid_string);
    LocalFree(sid_string);
    CloseHandle(token);

    
    size_t last_dash = sid.rfind('-');
    if (last_dash != std::string::npos) {
        sid = sid.substr(0, last_dash);
    }

    return sid;
#else
    
    
    namespace fs = std::filesystem;

    try {
        fs::path uuid_dir = "/dev/disk/by-uuid";
        if (!fs::exists(uuid_dir)) {
            return "";
        }

        
        std::map<std::string, std::string> device_to_uuid;
        for (const auto& entry : fs::directory_iterator(uuid_dir)) {
            if (fs::is_symlink(entry)) {
                std::string uuid = entry.path().filename().string();
                fs::path target = fs::read_symlink(entry.path());
                std::string device = target.filename().string();
                device_to_uuid[device] = uuid;
            }
        }

        
        std::vector<std::string> preferred_devices = {
            "sda", "sda1", "sdb", "sdb1", "sdc", "sdc1", "sdd", "sdd1",
            "hda", "hda1", "hdb", "hdb1",
            "dm-0", "dm-1", "nvme0n1", "nvme0n1p1"
        };

        for (const auto& dev : preferred_devices) {
            auto it = device_to_uuid.find(dev);
            if (it != device_to_uuid.end()) {
                return it->second;
            }
        }

        
        if (!device_to_uuid.empty()) {
            return device_to_uuid.begin()->second;
        }

        return "";
    } catch (...) {
        return "";
    }
#endif
}


inline std::string normalize_path(const std::string& path) {
    
    
    std::filesystem::path canon_path = std::filesystem::canonical(path);
    std::string result = canon_path.string();

#ifdef _WIN32
    if (result.size() >= 2 && result[1] == ':') {
        result[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(result[0])));
    }
    std::replace(result.begin(), result.end(), '/', '\\');
#endif

    return result;
}


inline std::vector<uint8_t> path_to_bytes(const std::string& path) {
#ifdef _WIN32
    int wide_len = MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, nullptr, 0);
    if (wide_len <= 0) return {};

    std::vector<wchar_t> wide_str(wide_len);
    MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, wide_str.data(), wide_len);
    wide_len--;

    std::vector<uint8_t> result(wide_len * 2);
    for (int i = 0; i < wide_len; ++i) {
        result[i * 2] = static_cast<uint8_t>(wide_str[i] & 0xFF);
        result[i * 2 + 1] = static_cast<uint8_t>((wide_str[i] >> 8) & 0xFF);
    }
    return result;
#else
    return std::vector<uint8_t>(path.begin(), path.end());
#endif
}


inline std::string chrome_time_now() {
#ifdef _WIN32
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    uint64_t time = (static_cast<uint64_t>(ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
    time /= 10;  
    return std::to_string(time);
#else
    constexpr uint64_t EPOCH_DIFF = 11644473600000000ULL;
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto micros = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
    return std::to_string(static_cast<uint64_t>(micros) + EPOCH_DIFF);
#endif
}

inline std::string read_file(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + path);
    }
    auto size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::string content(size, '\0');
    file.read(content.data(), size);
    return content;
}

inline void write_file(const std::string& path, const std::string& content) {
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to create file: " + path);
    }
    file.write(content.data(), content.size());
}

inline void create_directories(const std::string& path) {
    std::filesystem::create_directories(path);
}

} 


namespace json_util {


inline std::string serialize_for_mac(const JsonValue& value);

inline void serialize_string_for_mac(const std::string& str, std::string& out) {
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
            case '<': out += "\\u003C"; break;
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

inline void serialize_value_for_mac(const JsonValue& value, std::string& out) {
    if (value.isNull()) {
        out += "null";
    } else if (value.isBool()) {
        out += value.asBool() ? "true" : "false";
    } else if (value.isInt()) {
        out += std::to_string(value.asInt64());
    } else if (value.isDouble()) {
        out += std::to_string(static_cast<int64_t>(value.asDouble()));
    } else if (value.isString()) {
        serialize_string_for_mac(value.asString(), out);
    } else if (value.isArray()) {
        out += '[';
        bool first = true;
        for (const auto& elem : value.asArray()) {
            if (!first) out += ',';
            first = false;
            serialize_value_for_mac(elem, out);
        }
        out += ']';
    } else if (value.isObject()) {
        out += '{';
        bool first = true;
        for (const auto& [key, val] : value.asObject()) {
            if (!first) out += ',';
            first = false;
            serialize_string_for_mac(key, out);
            out += ':';
            serialize_value_for_mac(val, out);
        }
        out += '}';
    }
}

inline std::string serialize_for_mac(const JsonValue& value) {
    std::string result;
    serialize_value_for_mac(value, result);
    return result;
}


inline JsonValue remove_empty_entries(const JsonValue& value) {
    if (value.isObject()) {
        JsonObject result;
        for (const auto& [key, val] : value.asObject()) {
            JsonValue cleaned = remove_empty_entries(val);
            if (cleaned.isObject() && cleaned.asObject().empty()) continue;
            if (cleaned.isArray() && cleaned.asArray().empty()) continue;
            if (cleaned.isString() && cleaned.asString().empty()) continue;
            result[key] = std::move(cleaned);
        }
        return JsonValue(std::move(result));
    } else if (value.isArray()) {
        JsonArray result;
        for (const auto& elem : value.asArray()) {
            result.push_back(remove_empty_entries(elem));
        }
        return JsonValue(std::move(result));
    }
    return value;
}

} 


inline std::string generate_extension_id(const std::string& extension_path) {
    std::string normalized = platform::normalize_path(extension_path);
    std::vector<uint8_t> path_bytes = platform::path_to_bytes(normalized);

    auto hash = SHA256::hash(path_bytes.data(), path_bytes.size());
    std::string hash_hex = SHA256::to_hex(hash);

    std::string id;
    id.reserve(32);
    for (int i = 0; i < 32; ++i) {
        char c = hash_hex[i];
        int val = (c >= '0' && c <= '9') ? (c - '0') : (c - 'a' + 10);
        id += static_cast<char>('a' + val);
    }
    return id;
}


inline std::string generate_extension_id_from_key(const std::string& key) {
    std::vector<uint8_t> key_bytes = Base64::decode(key);
    auto hash = SHA256::hash(key_bytes.data(), key_bytes.size());
    std::string hash_hex = SHA256::to_hex(hash);

    std::string id;
    id.reserve(32);
    for (int i = 0; i < 32; ++i) {
        char c = hash_hex[i];
        int val = (c >= '0' && c <= '9') ? (c - '0') : (c - 'a' + 10);
        id += static_cast<char>('a' + val);
    }
    return id;
}


inline const std::vector<uint8_t> CHROME_SEED = {
    0xe7, 0x48, 0xf3, 0x36, 0xd8, 0x5e, 0xa5, 0xf9, 0xdc, 0xdf, 0x25, 0xd8, 0xf3, 0x47, 0xa6, 0x5b,
    0x4c, 0xdf, 0x66, 0x76, 0x00, 0xf0, 0x2d, 0xf6, 0x72, 0x4a, 0x2a, 0xf1, 0x8a, 0x21, 0x2d, 0x26,
    0xb7, 0x88, 0xa2, 0x50, 0x86, 0x91, 0x0c, 0xf3, 0xa9, 0x03, 0x13, 0x69, 0x68, 0x71, 0xf3, 0xdc,
    0x05, 0x82, 0x37, 0x30, 0xc9, 0x1d, 0xf8, 0xba, 0x5c, 0x4f, 0xd9, 0xc8, 0x84, 0xb5, 0x05, 0xa8
};


struct ExtensionInfo {
    std::string id;                         
    std::string path;                       
    std::string name;                       
    std::string version;                    
    std::vector<std::string> permissions;   
    std::vector<std::string> hostPermissions; 
    bool incognito = false;                 
    bool fileAccess = false;                
};

class ExtensionLoader {
public:
    
    static std::string calc_hmac(const std::string& message, const std::string& sid, const std::string& path = "") {
        std::string data = sid + path + message;
        return HMAC_SHA256::compute_hex(CHROME_SEED, data);
    }

    
    static JsonValue parse_manifest(const std::string& path) {
        std::string content = platform::read_file(path);
        return JsonValue::parse(content);
    }

    
    static std::vector<std::string> get_scriptable_hosts(const JsonValue& manifest) {
        std::set<std::string> hosts;
        if (manifest.contains("content_scripts")) {
            for (const auto& cs : manifest["content_scripts"].asArray()) {
                if (cs.contains("matches")) {
                    for (const auto& match : cs["matches"].asArray()) {
                        hosts.insert(match.asString());
                    }
                }
            }
        }
        return std::vector<std::string>(hosts.begin(), hosts.end());
    }

    
    static std::vector<std::string> get_api_permissions(const JsonValue& manifest) {
        std::set<std::string> perms;
        if (manifest.contains("permissions")) {
            for (const auto& perm : manifest["permissions"].asArray()) {
                perms.insert(perm.asString());
            }
        }
        return std::vector<std::string>(perms.begin(), perms.end());
    }

    
    static std::vector<std::string> get_explicit_hosts(const JsonValue& manifest) {
        std::set<std::string> hosts;
        if (manifest.contains("host_permissions")) {
            for (const auto& host : manifest["host_permissions"].asArray()) {
                hosts.insert(host.asString());
            }
        }
        return std::vector<std::string>(hosts.begin(), hosts.end());
    }

    
    static JsonValue build_extension_entry(
        const std::string& ext_path,
        const JsonValue& manifest,
        bool incognito,
        bool file_access
    ) {
        auto scriptable_hosts = get_scriptable_hosts(manifest);
        auto api_permissions = get_api_permissions(manifest);
        auto explicit_hosts = get_explicit_hosts(manifest);
        std::string version = manifest["version"].getString("1.0");
        std::string now = platform::chrome_time_now();

        JsonArray api_arr, explicit_arr, scriptable_arr;
        for (const auto& p : api_permissions) api_arr.push_back(JsonValue(p));
        for (const auto& h : explicit_hosts) explicit_arr.push_back(JsonValue(h));
        for (const auto& h : scriptable_hosts) scriptable_arr.push_back(JsonValue(h));

        JsonObject active_permissions;
        active_permissions["api"] = JsonValue(std::move(api_arr));
        active_permissions["explicit_host"] = JsonValue(std::move(explicit_arr));
        active_permissions["manifest_permissions"] = JsonValue(JsonArray{});
        active_permissions["scriptable_host"] = JsonValue(std::move(scriptable_arr));

        
        JsonArray api_arr2, explicit_arr2, scriptable_arr2;
        for (const auto& p : api_permissions) api_arr2.push_back(JsonValue(p));
        for (const auto& h : explicit_hosts) explicit_arr2.push_back(JsonValue(h));
        for (const auto& h : scriptable_hosts) scriptable_arr2.push_back(JsonValue(h));

        JsonObject granted_permissions;
        granted_permissions["api"] = JsonValue(std::move(api_arr2));
        granted_permissions["explicit_host"] = JsonValue(std::move(explicit_arr2));
        granted_permissions["manifest_permissions"] = JsonValue(JsonArray{});
        granted_permissions["scriptable_host"] = JsonValue(std::move(scriptable_arr2));

        JsonObject sw_info;
        sw_info["version"] = JsonValue(version);

        JsonObject entry;
        entry["account_extension_type"] = JsonValue(0);
        entry["active_permissions"] = JsonValue(std::move(active_permissions));
        entry["commands"] = JsonValue(JsonObject{});
        entry["content_settings"] = JsonValue(JsonArray{});
        entry["creation_flags"] = JsonValue(38);
        entry["disable_reasons"] = JsonValue(JsonArray{});
        entry["first_install_time"] = JsonValue(now);
        entry["from_webstore"] = JsonValue(false);
        entry["granted_permissions"] = JsonValue(std::move(granted_permissions));
        entry["incognito_content_settings"] = JsonValue(JsonArray{});
        entry["incognito_preferences"] = JsonValue(JsonObject{});
        entry["last_update_time"] = JsonValue(now);
        entry["location"] = JsonValue(4);
        entry["path"] = JsonValue(ext_path);
        entry["preferences"] = JsonValue(JsonObject{});
        entry["regular_only_preferences"] = JsonValue(JsonObject{});
        entry["service_worker_registration_info"] = JsonValue(std::move(sw_info));
        entry["serviceworkerevents"] = JsonValue(JsonArray{});
        entry["was_installed_by_default"] = JsonValue(false);
        entry["was_installed_by_oem"] = JsonValue(false);
        entry["withholding_permissions"] = JsonValue(false);

        if (incognito) {
            entry["incognito"] = JsonValue(true);
        }
        if (file_access) {
            entry["newAllowFileAccess"] = JsonValue(true);
        }

        return JsonValue(std::move(entry));
    }

    
    static std::vector<ExtensionInfo> loadExtensions(
        const std::string& user_data_dir,
        const std::vector<std::string>& extension_paths,
        bool incognito = true,
        bool file_access = true
    ) {
        namespace fs = std::filesystem;

        if (extension_paths.empty()) {
            return {};
        }

        fs::path user_data_path(user_data_dir);
        fs::path default_path = user_data_path / "Default";
        platform::create_directories(default_path.string());

        std::string sid = platform::get_sid();
        if (sid.empty()) {
            throw std::runtime_error("Failed to get system identifier for extension signing");
        }

        std::vector<ExtensionInfo> loaded_extensions;
        JsonObject ext_settings;

        for (const auto& extension_path : extension_paths) {
            std::string ext_path = platform::normalize_path(extension_path);
            fs::path manifest_path = fs::path(ext_path) / "manifest.json";

            if (!fs::exists(manifest_path)) {
                throw std::runtime_error("Extension manifest not found: " + manifest_path.string());
            }

            JsonValue manifest = parse_manifest(manifest_path.string());

            
            std::string ext_id;
            if (manifest.contains("key")) {
                ext_id = generate_extension_id_from_key(manifest["key"].asString());
            } else {
                ext_id = generate_extension_id(ext_path);
            }

            
            ExtensionInfo info;
            info.id = ext_id;
            info.path = ext_path;
            info.name = manifest["name"].getString("Unknown Extension");
            info.version = manifest["version"].getString("1.0");
            info.permissions = get_api_permissions(manifest);
            info.hostPermissions = get_explicit_hosts(manifest);
            info.incognito = incognito;
            info.fileAccess = file_access;
            loaded_extensions.push_back(info);

            
            JsonValue ext_entry = build_extension_entry(ext_path, manifest, incognito, file_access);
            ext_settings[ext_id] = ext_entry;
        }

#ifdef _WIN32
        
        JsonObject ext_macs;
        for (const auto& [ext_id, ext_entry] : ext_settings) {
            JsonValue ext_for_mac = json_util::remove_empty_entries(ext_entry);
            std::string mac_json = json_util::serialize_for_mac(ext_for_mac);
            std::string mac_path = "extensions.settings." + ext_id;
            std::string ext_mac = calc_hmac(mac_json, sid, mac_path);
            ext_macs[ext_id] = JsonValue(ext_mac);
        }

        
        std::string dev_mac = calc_hmac("true", sid, "extensions.ui.developer_mode");

        
        JsonObject ui_macs;
        ui_macs["developer_mode"] = JsonValue(dev_mac);

        JsonObject extensions_macs;
        extensions_macs["settings"] = JsonValue(std::move(ext_macs));
        extensions_macs["ui"] = JsonValue(std::move(ui_macs));

        JsonObject macs_obj;
        macs_obj["extensions"] = JsonValue(std::move(extensions_macs));

        
        std::string macs_json = JsonValue(macs_obj).serialize();
        std::string super_mac = calc_hmac(macs_json, sid);

        
        JsonObject ui_settings;
        ui_settings["developer_mode"] = JsonValue(true);

        JsonObject extensions;
        extensions["settings"] = JsonValue(std::move(ext_settings));
        extensions["ui"] = JsonValue(std::move(ui_settings));

        JsonObject protection;
        protection["macs"] = JsonValue(std::move(macs_obj));
        protection["super_mac"] = JsonValue(super_mac);

        JsonObject secure_preferences;
        secure_preferences["extensions"] = JsonValue(std::move(extensions));
        secure_preferences["protection"] = JsonValue(std::move(protection));

        
        fs::path secure_prefs_path = default_path / "Secure Preferences";
        std::string secure_prefs_json = JsonValue(std::move(secure_preferences)).serialize();
        platform::write_file(secure_prefs_path.string(), secure_prefs_json);

        
        fs::path prefs_path = default_path / "Preferences";
        platform::write_file(prefs_path.string(), "{}");

#else
        

        JsonObject ui_settings;
        ui_settings["developer_mode"] = JsonValue(true);

        JsonObject extensions;
        extensions["settings"] = JsonValue(std::move(ext_settings));
        extensions["ui"] = JsonValue(std::move(ui_settings));

        JsonObject preferences;
        preferences["extensions"] = JsonValue(std::move(extensions));

        
        fs::path prefs_path = default_path / "Preferences";
        std::string prefs_json = JsonValue(preferences).serialize();
        platform::write_file(prefs_path.string(), prefs_json);

        
        std::string super_mac = HMAC_SHA256::compute_hex(CHROME_SEED, "");

        
        JsonObject protection;
        protection["super_mac"] = JsonValue(super_mac);

        JsonObject secure_preferences;
        secure_preferences["protection"] = JsonValue(std::move(protection));

        
        fs::path secure_prefs_path = default_path / "Secure Preferences";
        std::string secure_prefs_json = JsonValue(std::move(secure_preferences)).serialize();
        platform::write_file(secure_prefs_path.string(), secure_prefs_json);
#endif

        
        JsonObject default_info;
        default_info["name"] = JsonValue("Default");

        JsonObject info_cache;
        info_cache["Default"] = JsonValue(std::move(default_info));

        JsonObject profile;
        profile["info_cache"] = JsonValue(std::move(info_cache));

        JsonObject local_state;
        local_state["profile"] = JsonValue(std::move(profile));

        fs::path local_state_path = user_data_path / "Local State";
        std::string local_state_json = JsonValue(std::move(local_state)).serialize();
        platform::write_file(local_state_path.string(), local_state_json);

        return loaded_extensions;
    }

    
    static bool isTempUserDataDir(const std::string& path, const std::string& tempPrefix = "cdp_chrome_") {
        std::filesystem::path p(path);
        std::string dirname = p.filename().string();
        return dirname.find(tempPrefix) == 0;
    }
};

} 
} 
