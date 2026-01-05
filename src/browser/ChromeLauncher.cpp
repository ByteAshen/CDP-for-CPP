

#ifdef _WIN32

#include "cdp/net/Socket.hpp"  
#include <shlobj.h>
#include <tlhelp32.h>
#pragma comment(lib, "version.lib")

#include "cdp/browser/ChromeLauncher.hpp"
#include "cdp/net/HttpClient.hpp"
#include "cdp/core/Json.hpp"
#include "cdp/browser/ExtensionLoader.hpp"
#include <algorithm>
#include <thread>
#include <random>
#include <sstream>
#include <cstdlib>
#include <iostream>

namespace cdp {


std::vector<std::string> ChromeLaunchOptions::buildArgs() const {
    std::vector<std::string> args;

    
    args.push_back("--remote-debugging-port=" + std::to_string(debuggingPort));
    args.push_back("--remote-debugging-address=" + host);

    
    if (!userDataDir.empty()) {
        args.push_back("--user-data-dir=" + userDataDir);
    }

    
    if (headless) {
        args.push_back("--headless=new");  
    }

    if (startMaximized) {
        args.push_back("--start-maximized");
    } else if (windowWidth > 0 && windowHeight > 0) {
        args.push_back("--window-size=" + std::to_string(windowWidth) + "," +
                       std::to_string(windowHeight));
    }

    if (windowX >= 0 && windowY >= 0) {
        args.push_back("--window-position=" + std::to_string(windowX) + "," +
                       std::to_string(windowY));
    }

    
    if (disableGpu) {
        args.push_back("--disable-gpu");
        args.push_back("--disable-software-rasterizer");
    }

    if (disableExtensions) {
        args.push_back("--disable-extensions");
    }

    if (disablePopupBlocking) {
        args.push_back("--disable-popup-blocking");
    }

    if (disableDefaultApps) {
        args.push_back("--disable-default-apps");
    }

    if (noFirstRun) {
        args.push_back("--no-first-run");
        args.push_back("--no-default-browser-check");
    }

    if (noDefaultBrowserCheck) {
        args.push_back("--no-default-browser-check");
    }

    if (disableBackgroundNetworking) {
        args.push_back("--disable-background-networking");
    }

    if (disableSync) {
        args.push_back("--disable-sync");
    }

    if (disableTranslate) {
        args.push_back("--disable-translate");
    }

    if (muteAudio) {
        args.push_back("--mute-audio");
    }

    if (ignoreSSLErrors) {
        args.push_back("--ignore-certificate-errors");
        args.push_back("--ignore-ssl-errors");
    }

    
    if (!proxyServer.empty()) {
        args.push_back("--proxy-server=" + proxyServer);
    }

    if (!proxyBypassList.empty()) {
        args.push_back("--proxy-bypass-list=" + proxyBypassList);
    }

    
    args.push_back("--disable-hang-monitor");
    args.push_back("--disable-ipc-flooding-protection");
    args.push_back("--disable-prompt-on-repost");
    args.push_back("--disable-renderer-backgrounding");
    args.push_back("--disable-backgrounding-occluded-windows");
    args.push_back("--disable-component-update");
    args.push_back("--disable-breakpad");  
    args.push_back("--metrics-recording-only");  
    args.push_back("--safebrowsing-disable-auto-update");
    args.push_back("--password-store=basic");  
    args.push_back("--use-mock-keychain");  

    
    args.push_back("--enable-features=NetworkService,NetworkServiceInProcess");

    
    for (const auto& flag : additionalFlags) {
        args.push_back(flag);
    }

    
    if (!startUrl.empty()) {
        args.push_back(startUrl);
    }

    return args;
}


ChromeLauncher::ChromeLauncher() : ChromeLauncher(ChromeLaunchOptions{}) {}

ChromeLauncher::ChromeLauncher(const ChromeLaunchOptions& options)
    : options_(options) {}

ChromeLauncher::~ChromeLauncher() {
    if (options_.killOnDestruct && launched_.load()) {
        kill();
    }

    if (options_.cleanupTempProfile && options_.useTempProfile) {
        cleanupTempProfile();
    }

#ifdef _WIN32
    if (processHandle_) {
        CloseHandle(processHandle_);
        processHandle_ = nullptr;
    }
    if (threadHandle_) {
        CloseHandle(threadHandle_);
        threadHandle_ = nullptr;
    }
#endif
}

ChromeLauncher::ChromeLauncher(ChromeLauncher&& other) noexcept
    : options_(std::move(other.options_))
    , installation_(std::move(other.installation_))
    , userDataDir_(std::move(other.userDataDir_))
    , lastError_(std::move(other.lastError_))
    , launched_(other.launched_.load())
#ifdef _WIN32
    , processHandle_(other.processHandle_)
    , threadHandle_(other.threadHandle_)
    , processId_(other.processId_)
#endif
{
#ifdef _WIN32
    other.processHandle_ = nullptr;
    other.threadHandle_ = nullptr;
    other.processId_ = 0;
#endif
    other.launched_ = false;
}

ChromeLauncher& ChromeLauncher::operator=(ChromeLauncher&& other) noexcept {
    if (this != &other) {
        
        if (options_.killOnDestruct && launched_.load()) {
            kill();
        }
#ifdef _WIN32
        if (processHandle_) CloseHandle(processHandle_);
        if (threadHandle_) CloseHandle(threadHandle_);
#endif

        
        options_ = std::move(other.options_);
        installation_ = std::move(other.installation_);
        userDataDir_ = std::move(other.userDataDir_);
        lastError_ = std::move(other.lastError_);
        launched_ = other.launched_.load();
#ifdef _WIN32
        processHandle_ = other.processHandle_;
        threadHandle_ = other.threadHandle_;
        processId_ = other.processId_;
        other.processHandle_ = nullptr;
        other.threadHandle_ = nullptr;
        other.processId_ = 0;
#endif
        other.launched_ = false;
    }
    return *this;
}


std::vector<ChromeInstallation> ChromeLauncher::findAllInstallations() {
    std::vector<ChromeInstallation> installations;

#ifdef _WIN32
    
    auto getEnvPath = [](const char* var) -> std::string {
        char buffer[MAX_PATH];
        if (GetEnvironmentVariableA(var, buffer, MAX_PATH) > 0) {
            return std::string(buffer);
        }
        return "";
    };

    
    auto getSpecialFolder = [](int csidl) -> std::string {
        char path[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathA(nullptr, csidl, nullptr, SHGFP_TYPE_CURRENT, path))) {
            return std::string(path);
        }
        return "";
    };

    std::string programFiles = getEnvPath("ProgramFiles");
    std::string programFilesX86 = getEnvPath("ProgramFiles(x86)");
    std::string localAppData = getSpecialFolder(CSIDL_LOCAL_APPDATA);
    std::string appData = getSpecialFolder(CSIDL_APPDATA);
    std::string userProfile = getEnvPath("USERPROFILE");

    
    struct PathInfo {
        std::string path;
        ChromeChannel channel;
    };

    std::vector<PathInfo> pathsToCheck;

    
    auto addPaths = [&pathsToCheck](const std::string& base) {
        if (base.empty()) return;

        
        pathsToCheck.push_back({base + "\\Google\\Chrome\\Application\\chrome.exe", ChromeChannel::Stable});
        pathsToCheck.push_back({base + "\\Google\\Chrome Beta\\Application\\chrome.exe", ChromeChannel::Beta});
        pathsToCheck.push_back({base + "\\Google\\Chrome Dev\\Application\\chrome.exe", ChromeChannel::Dev});
        pathsToCheck.push_back({base + "\\Google\\Chrome SxS\\Application\\chrome.exe", ChromeChannel::Canary});
        pathsToCheck.push_back({base + "\\Chromium\\Application\\chrome.exe", ChromeChannel::Chromium});

        
        pathsToCheck.push_back({base + "\\Google\\Chrome\\chrome.exe", ChromeChannel::Stable});
        pathsToCheck.push_back({base + "\\Chrome\\Application\\chrome.exe", ChromeChannel::Stable});
        pathsToCheck.push_back({base + "\\Chromium\\chrome.exe", ChromeChannel::Chromium});
    };

    
    addPaths(programFiles);
    addPaths(programFilesX86);
    addPaths(localAppData);
    addPaths(appData);

    
    if (!userProfile.empty()) {
        addPaths(userProfile + "\\AppData\\Local");
        addPaths(userProfile + "\\scoop\\apps\\googlechrome\\current");  
        addPaths(userProfile + "\\scoop\\apps\\chromium\\current");
    }

    
    std::string pathEnv = getEnvPath("PATH");
    if (!pathEnv.empty()) {
        std::istringstream pathStream(pathEnv);
        std::string pathEntry;
        while (std::getline(pathStream, pathEntry, ';')) {
            if (!pathEntry.empty()) {
                pathsToCheck.push_back({pathEntry + "\\chrome.exe", ChromeChannel::Stable});
                pathsToCheck.push_back({pathEntry + "\\chromium.exe", ChromeChannel::Chromium});
            }
        }
    }

    
    for (const auto& info : pathsToCheck) {
        try {
            std::filesystem::path p(info.path);
            if (std::filesystem::exists(p) && std::filesystem::is_regular_file(p)) {
                ChromeInstallation install;
                install.path = info.path;
                install.channel = info.channel;
                install.lastModified = std::filesystem::last_write_time(p);
                install.version = getChromeVersion(info.path);

                
                bool duplicate = false;
                for (const auto& existing : installations) {
                    if (std::filesystem::equivalent(existing.path, info.path)) {
                        duplicate = true;
                        break;
                    }
                }

                if (!duplicate) {
                    installations.push_back(install);
                }
            }
        } catch (const std::filesystem::filesystem_error&) {
            
        }
    }

    
    auto checkRegistry = [&installations](HKEY root, const char* subkey, ChromeChannel channel) {
        HKEY key;
        if (RegOpenKeyExA(root, subkey, 0, KEY_READ, &key) == ERROR_SUCCESS) {
            char path[MAX_PATH];
            DWORD pathSize = sizeof(path);
            if (RegQueryValueExA(key, nullptr, nullptr, nullptr,
                                  reinterpret_cast<LPBYTE>(path), &pathSize) == ERROR_SUCCESS) {
                
                std::string fullPath(path);
                size_t quoteEnd = 0;
                if (!fullPath.empty() && fullPath[0] == '"') {
                    quoteEnd = fullPath.find('"', 1);
                    if (quoteEnd != std::string::npos) {
                        fullPath = fullPath.substr(1, quoteEnd - 1);
                    }
                } else {
                    size_t spacePos = fullPath.find(' ');
                    if (spacePos != std::string::npos) {
                        fullPath = fullPath.substr(0, spacePos);
                    }
                }

                try {
                    if (std::filesystem::exists(fullPath)) {
                        bool duplicate = false;
                        for (const auto& existing : installations) {
                            if (std::filesystem::equivalent(existing.path, fullPath)) {
                                duplicate = true;
                                break;
                            }
                        }
                        if (!duplicate) {
                            ChromeInstallation install;
                            install.path = fullPath;
                            install.channel = channel;
                            install.lastModified = std::filesystem::last_write_time(fullPath);
                            install.version = getChromeVersion(fullPath);
                            installations.push_back(install);
                        }
                    }
                } catch (const std::exception&) {
                    
                }
            }
            RegCloseKey(key);
        }
    };

    
    checkRegistry(HKEY_LOCAL_MACHINE,
        "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\chrome.exe",
        ChromeChannel::Stable);
    checkRegistry(HKEY_CURRENT_USER,
        "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\chrome.exe",
        ChromeChannel::Stable);

#endif 

    
    std::sort(installations.begin(), installations.end(),
        [](const ChromeInstallation& a, const ChromeInstallation& b) {
            return a.lastModified > b.lastModified;
        });

    return installations;
}

std::optional<ChromeInstallation> ChromeLauncher::findBestInstallation() {
    auto installations = findAllInstallations();
    if (installations.empty()) {
        return std::nullopt;
    }

    
    auto stableIt = std::find_if(installations.begin(), installations.end(),
        [](const ChromeInstallation& i) { return i.channel == ChromeChannel::Stable; });

    if (stableIt != installations.end()) {
        return *stableIt;
    }

    
    return installations.front();
}

std::optional<ChromeInstallation> ChromeLauncher::findInstallation(ChromeChannel channel) {
    auto installations = findAllInstallations();

    for (const auto& install : installations) {
        if (install.channel == channel) {
            return install;
        }
    }

    return std::nullopt;
}

bool ChromeLauncher::isValidChrome(const std::string& path) {
    if (path.empty()) return false;

    try {
        std::filesystem::path p(path);
        if (!std::filesystem::exists(p) || !std::filesystem::is_regular_file(p)) {
            return false;
        }

        
        std::string filename = p.filename().string();
        std::transform(filename.begin(), filename.end(), filename.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return filename.find("chrome") != std::string::npos ||
               filename.find("chromium") != std::string::npos;
    } catch (const std::exception&) {
        return false;
    }
}

std::string ChromeLauncher::getChromeVersion(const std::string& path) {
#ifdef _WIN32
    DWORD dummy;
    DWORD size = GetFileVersionInfoSizeA(path.c_str(), &dummy);
    if (size == 0) return "";

    std::vector<char> buffer(size);
    if (!GetFileVersionInfoA(path.c_str(), 0, size, buffer.data())) {
        return "";
    }

    VS_FIXEDFILEINFO* fileInfo = nullptr;
    UINT len;
    if (!VerQueryValueA(buffer.data(), "\\", reinterpret_cast<LPVOID*>(&fileInfo), &len)) {
        return "";
    }

    std::ostringstream oss;
    oss << HIWORD(fileInfo->dwFileVersionMS) << "."
        << LOWORD(fileInfo->dwFileVersionMS) << "."
        << HIWORD(fileInfo->dwFileVersionLS) << "."
        << LOWORD(fileInfo->dwFileVersionLS);
    return oss.str();
#else
    return "";
#endif
}


int ChromeLauncher::findFreePort() {
#ifdef _WIN32
    
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        return 0;
    }

    
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        WSACleanup();
        return 0;
    }

    
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0;  

    if (bind(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
        closesocket(sock);
        WSACleanup();
        return 0;
    }

    
    int addrLen = sizeof(addr);
    if (getsockname(sock, reinterpret_cast<sockaddr*>(&addr), &addrLen) == SOCKET_ERROR) {
        closesocket(sock);
        WSACleanup();
        return 0;
    }

    int port = ntohs(addr.sin_port);

    
    closesocket(sock);
    WSACleanup();

    return port;
#else
    
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return 0;

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0;

    if (bind(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        close(sock);
        return 0;
    }

    socklen_t addrLen = sizeof(addr);
    if (getsockname(sock, reinterpret_cast<sockaddr*>(&addr), &addrLen) < 0) {
        close(sock);
        return 0;
    }

    int port = ntohs(addr.sin_port);
    close(sock);
    return port;
#endif
}


bool ChromeLauncher::launch() {
    return launch(options_);
}

bool ChromeLauncher::launch(const ChromeLaunchOptions& options) {
    options_ = options;
    lastError_.clear();

    
    if (options_.debuggingPort == 0) {
        int freePort = findFreePort();
        if (freePort == 0) {
            lastError_ = "Failed to find a free port for CDP debugging";
            return false;
        }
        options_.debuggingPort = freePort;
    }

    
    if (options_.preferredChannel == ChromeChannel::Custom) {
        if (options_.customChromePath.empty()) {
            lastError_ = "Custom Chrome path not specified";
            return false;
        }
        installation_.path = options_.customChromePath;
        installation_.channel = ChromeChannel::Custom;
    } else {
        auto install = findInstallation(options_.preferredChannel);
        if (!install) {
            
            install = findBestInstallation();
        }

        if (!install) {
            lastError_ = "No Chrome installation found on the system";
            return false;
        }

        installation_ = *install;
    }

    
    if (!isValidChrome(installation_.path)) {
        lastError_ = "Invalid Chrome executable: " + installation_.path;
        return false;
    }

    
    if (options_.useTempProfile) {
        if (!createTempProfile()) {
            return false;
        }
        
        options_.userDataDir = userDataDir_;
    } else if (!options_.userDataDir.empty()) {
        userDataDir_ = options_.userDataDir;
    }

    
    if (!options_.extensions.empty()) {
        
        if (!options_.useTempProfile && !options_.allowExtensionsOnCustomDir) {
            lastError_ = "Extension loading is only allowed on CDP-managed temp directories. "
                        "Using a custom user data directory with extensions requires "
                        "allowExtensionsOnCustomDir=true. WARNING: This is risky because "
                        "corrupted Secure Preferences can cause Chrome to reset your profile, "
                        "losing bookmarks, history, and saved passwords.";
            return false;
        }

        
        if (!options_.useTempProfile && options_.allowExtensionsOnCustomDir) {
            
            
            std::cerr << "[CDP WARNING] Loading extensions into custom user data directory: "
                      << userDataDir_ << "\n"
                      << "              This modifies Secure Preferences. Ensure you have backups!\n";
        }

        
        try {
            extension::ExtensionLoader::loadExtensions(
                userDataDir_,
                options_.extensions,
                options_.extensionIncognitoEnabled,
                options_.extensionFileAccessEnabled
            );

            
            if (options_.disableExtensions) {
                options_.disableExtensions = false;
            }
        } catch (const std::exception& e) {
            lastError_ = "Failed to load extensions: " + std::string(e.what());
            return false;
        }
    }

    
    auto args = options_.buildArgs();

    
    if (!startProcess(installation_.path, args)) {
        return false;
    }

    launched_ = true;

    
    if (!waitForReady(options_.maxStartupWaitMs)) {
        lastError_ = "Chrome started but CDP endpoint not available after " +
                     std::to_string(options_.maxStartupWaitMs) + "ms";
        return false;
    }

    return true;
}

bool ChromeLauncher::startProcess(const std::string& chromePath,
                                   const std::vector<std::string>& args) {
#ifdef _WIN32
    
    std::ostringstream cmdLine;
    cmdLine << "\"" << chromePath << "\"";
    for (const auto& arg : args) {
        cmdLine << " ";
        
        if (arg.find(' ') != std::string::npos && arg[0] != '"') {
            cmdLine << "\"" << arg << "\"";
        } else {
            cmdLine << arg;
        }
    }

    std::string cmdLineStr = cmdLine.str();

    
    STARTUPINFOA si = {};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = options_.headless ? SW_HIDE : SW_SHOWNORMAL;

    PROCESS_INFORMATION pi = {};

    
    std::vector<char> cmdBuffer(cmdLineStr.begin(), cmdLineStr.end());
    cmdBuffer.push_back('\0');

    BOOL success = CreateProcessA(
        nullptr,            
        cmdBuffer.data(),   
        nullptr,            
        nullptr,            
        FALSE,              
        CREATE_NEW_PROCESS_GROUP,  
        nullptr,            
        nullptr,            
        &si,               
        &pi                
    );

    if (!success) {
        DWORD error = GetLastError();
        lastError_ = "Failed to start Chrome (error " + std::to_string(error) + ")";
        return false;
    }

    processHandle_ = pi.hProcess;
    threadHandle_ = pi.hThread;
    processId_ = pi.dwProcessId;

    return true;
#else
    lastError_ = "Chrome launching not supported on this platform";
    return false;
#endif
}

bool ChromeLauncher::createTempProfile() {
#ifdef _WIN32
    
    cleanupStaleTempProfiles(options_.tempProfilePrefix);

    
    char tempPath[MAX_PATH];
    DWORD len = GetTempPathA(MAX_PATH, tempPath);
    if (len == 0 || len > MAX_PATH) {
        lastError_ = "Failed to get temp directory";
        return false;
    }

    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(100000, 999999);

    std::string uniqueFolder = options_.tempProfilePrefix + std::to_string(dis(gen));
    userDataDir_ = std::string(tempPath) + uniqueFolder;

    
    try {
        std::filesystem::create_directories(userDataDir_);
    } catch (const std::filesystem::filesystem_error& e) {
        lastError_ = "Failed to create temp profile directory: " + std::string(e.what());
        return false;
    }

    return true;
#else
    lastError_ = "Temp profile creation not supported on this platform";
    return false;
#endif
}

void ChromeLauncher::cleanupTempProfile() {
    if (userDataDir_.empty() || !options_.useTempProfile) {
        return;
    }

    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    try {
        std::filesystem::remove_all(userDataDir_);
    } catch (const std::exception&) {
        
    }
}

int ChromeLauncher::cleanupStaleTempProfiles(const std::string& prefix) {
    int cleanedCount = 0;

#ifdef _WIN32
    
    char tempPath[MAX_PATH];
    DWORD len = GetTempPathA(MAX_PATH, tempPath);
    if (len == 0 || len > MAX_PATH) {
        return 0;
    }

    std::filesystem::path tempDir(tempPath);

    try {
        for (const auto& entry : std::filesystem::directory_iterator(tempDir)) {
            if (!entry.is_directory()) {
                continue;
            }

            std::string folderName = entry.path().filename().string();

            
            if (folderName.find(prefix) != 0) {
                continue;
            }

            
            bool hasLockedFiles = false;

            try {
                
                
                for (const auto& subEntry : std::filesystem::recursive_directory_iterator(
                        entry.path(), std::filesystem::directory_options::skip_permission_denied)) {
                    if (subEntry.is_regular_file()) {
                        try {
                            std::filesystem::remove(subEntry.path());
                        } catch (const std::filesystem::filesystem_error&) {
                            
                            hasLockedFiles = true;
                            break;
                        }
                    }
                }

                if (hasLockedFiles) {
                    
                    continue;
                }

                
                std::filesystem::remove_all(entry.path());
                ++cleanedCount;

            } catch (const std::filesystem::filesystem_error&) {
                
                continue;
            }
        }
    } catch (const std::filesystem::filesystem_error&) {
        
    }

#endif 

    return cleanedCount;
}

bool ChromeLauncher::isRunning() const {
#ifdef _WIN32
    if (!processHandle_) return false;

    DWORD exitCode;
    if (GetExitCodeProcess(processHandle_, &exitCode)) {
        return exitCode == STILL_ACTIVE;
    }
    return false;
#else
    return false;
#endif
}

bool ChromeLauncher::waitForReady(int timeoutMs) {
    auto startTime = std::chrono::steady_clock::now();

    
    std::this_thread::sleep_for(std::chrono::milliseconds(options_.startupWaitMs));

    while (true) {
        
        if (!isRunning()) {
            lastError_ = "Chrome process exited unexpectedly";
            return false;
        }

        
        if (checkEndpointReady()) {
            return true;
        }

        
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - startTime).count();
        if (elapsed >= timeoutMs) {
            return false;
        }

        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

bool ChromeLauncher::checkEndpointReady() {
    HttpClient http;
    if (!http.connect(options_.host, options_.debuggingPort)) {
        return false;
    }

    auto response = http.get("/json/version");
    return response.isSuccess();
}

std::string ChromeLauncher::getDebugUrl() const {
    return "http://" + options_.host + ":" + std::to_string(options_.debuggingPort);
}

std::string ChromeLauncher::getBrowserWebSocketUrl() const {
    HttpClient http;
    if (!http.connect(options_.host, options_.debuggingPort)) {
        return "";
    }

    auto response = http.get("/json/version");
    if (!response.isSuccess()) {
        return "";
    }

    try {
        JsonValue json = JsonValue::parse(response.body);
        return json["webSocketDebuggerUrl"].getString();
    } catch (const std::exception&) {
        return "";
    }
}

void ChromeLauncher::kill() {
#ifdef _WIN32
    if (processHandle_) {
        TerminateProcess(processHandle_, 0);
        WaitForSingleObject(processHandle_, 5000);  
        CloseHandle(processHandle_);
        processHandle_ = nullptr;
    }
    if (threadHandle_) {
        CloseHandle(threadHandle_);
        threadHandle_ = nullptr;
    }
#endif
    launched_ = false;
}

bool ChromeLauncher::waitForExit(int timeoutMs) {
#ifdef _WIN32
    if (!processHandle_) return true;

    DWORD waitTime = (timeoutMs < 0) ? INFINITE : static_cast<DWORD>(timeoutMs);
    DWORD result = WaitForSingleObject(processHandle_, waitTime);
    return result == WAIT_OBJECT_0;
#else
    return true;
#endif
}

uint32_t ChromeLauncher::getProcessId() const {
#ifdef _WIN32
    return processId_;
#else
    return 0;
#endif
}

int ChromeLauncher::getExitCode() const {
#ifdef _WIN32
    if (!processHandle_) return -1;

    DWORD exitCode;
    if (GetExitCodeProcess(processHandle_, &exitCode)) {
        return static_cast<int>(exitCode);
    }
    return -1;
#else
    return -1;
#endif
}


std::optional<ChromeLauncher> launchChrome(const ChromeLaunchOptions& options) {
    ChromeLauncher launcher(options);
    if (launcher.launch()) {
        return std::move(launcher);
    }
    return std::nullopt;
}

std::optional<ChromeLauncher> launchHeadlessChrome(int port) {
    ChromeLaunchOptions options;
    options.headless = true;
    options.disableGpu = true;
    options.debuggingPort = port;
    return launchChrome(options);
}

} 

#endif 
