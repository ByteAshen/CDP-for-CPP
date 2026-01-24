#pragma once
#include <string>
#include <vector>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <chrono>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <sddl.h>
#pragma comment(lib, "advapi32.lib")
#endif

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
    try {
        std::ifstream f("/etc/machine-id");
        std::string id;
        std::getline(f, id);
        return id;
    } catch (...) {
        return "";
    }
#endif
}

inline std::string normalize_path(const std::string& path) {
    std::filesystem::path abs_path = std::filesystem::absolute(path);
    std::string result = abs_path.string();

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