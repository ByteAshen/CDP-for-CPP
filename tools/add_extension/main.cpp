#include "secure_preferences.hpp"
#include <iostream>
#include <vector>
#include <string>

void print_usage(const char* program) {
    std::cout << "Chrome Secure Preferences Generator (C++ Version)\n\n";
    std::cout << "Usage: " << program << " <user_data_dir> <ext_path1> [ext_path2 ...] [options]\n\n";
    std::cout << "Options:\n";
    std::cout << "  --no-incognito     Don't enable incognito mode\n";
    std::cout << "  --no-file-access   Don't enable file access\n\n";
    std::cout << "Example:\n";
    std::cout << "  " << program << " ./profile C:\\ext1 C:\\ext2\n";
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        print_usage(argv[0]);
        return 1;
    }

    std::string user_data_dir = argv[1];
    std::vector<std::string> extension_paths;
    bool incognito = true;
    bool file_access = true;

    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--no-incognito") {
            incognito = false;
        } else if (arg == "--no-file-access") {
            file_access = false;
        } else if (arg[0] != '-') {
            extension_paths.push_back(arg);
        }
    }

    if (extension_paths.empty()) {
        std::cerr << "Error: No extension paths provided\n";
        return 1;
    }

    try {
        chrome::create_secure_preferences(user_data_dir, extension_paths, incognito, file_access);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
