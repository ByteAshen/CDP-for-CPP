

#if !defined(_WIN32)

#include "cdp/browser/ChromeLauncher.hpp"
#include "cdp/net/HttpClient.hpp"
#include "cdp/net/Socket.hpp"
#include "cdp/core/Json.hpp"
#include "cdp/browser/ExtensionLoader.hpp"

#include <algorithm>
#include <thread>
#include <random>
#include <sstream>
#include <cstdlib>
#include <iostream>
#include <fstream>

#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pwd.h>

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
}

ChromeLauncher::ChromeLauncher(ChromeLauncher&& other) noexcept
    : options_(std::move(other.options_))
    , installation_(std::move(other.installation_))
    , userDataDir_(std::move(other.userDataDir_))
    , lastError_(std::move(other.lastError_))
    , launched_(other.launched_.load())
    , processId_(other.processId_)
{
    other.processId_ = 0;
    other.launched_ = false;
}

ChromeLauncher& ChromeLauncher::operator=(ChromeLauncher&& other) noexcept {
    if (this != &other) {
        if (options_.killOnDestruct && launched_.load()) {
            kill();
        }

        options_ = std::move(other.options_);
        installation_ = std::move(other.installation_);
        userDataDir_ = std::move(other.userDataDir_);
        lastError_ = std::move(other.lastError_);
        launched_ = other.launched_.load();
        processId_ = other.processId_;
        other.processId_ = 0;
        other.launched_ = false;
    }
    return *this;
}


std::vector<ChromeInstallation> ChromeLauncher::findAllInstallations() {
    std::vector<ChromeInstallation> installations;

    
    std::vector<std::pair<std::string, ChromeChannel>> linuxPaths = {
        
        {"/usr/bin/google-chrome", ChromeChannel::Stable},
        {"/usr/bin/google-chrome-stable", ChromeChannel::Stable},
        {"/usr/bin/google-chrome-beta", ChromeChannel::Beta},
        {"/usr/bin/google-chrome-unstable", ChromeChannel::Dev},
        {"/usr/bin/chromium", ChromeChannel::Chromium},
        {"/usr/bin/chromium-browser", ChromeChannel::Chromium},

        
        {"/snap/bin/chromium", ChromeChannel::Chromium},
        {"/snap/bin/google-chrome", ChromeChannel::Stable},

        
        {"/var/lib/flatpak/exports/bin/com.google.Chrome", ChromeChannel::Stable},
        {"/var/lib/flatpak/exports/bin/org.chromium.Chromium", ChromeChannel::Chromium},

        
        {"/opt/google/chrome/chrome", ChromeChannel::Stable},
        {"/opt/google/chrome-beta/chrome", ChromeChannel::Beta},
        {"/opt/google/chrome-unstable/chrome", ChromeChannel::Dev},
        {"/opt/chromium/chrome", ChromeChannel::Chromium},
    };

    
    std::vector<std::pair<std::string, ChromeChannel>> macosPaths = {
        {"/Applications/Google Chrome.app/Contents/MacOS/Google Chrome", ChromeChannel::Stable},
        {"/Applications/Google Chrome Beta.app/Contents/MacOS/Google Chrome Beta", ChromeChannel::Beta},
        {"/Applications/Google Chrome Dev.app/Contents/MacOS/Google Chrome Dev", ChromeChannel::Dev},
        {"/Applications/Google Chrome Canary.app/Contents/MacOS/Google Chrome Canary", ChromeChannel::Canary},
        {"/Applications/Chromium.app/Contents/MacOS/Chromium", ChromeChannel::Chromium},
    };

    
    const char* home = getenv("HOME");
    if (home) {
        std::string homeDir(home);
        macosPaths.push_back({homeDir + "/Applications/Google Chrome.app/Contents/MacOS/Google Chrome", ChromeChannel::Stable});
        macosPaths.push_back({homeDir + "/Applications/Chromium.app/Contents/MacOS/Chromium", ChromeChannel::Chromium});

        
        linuxPaths.push_back({homeDir + "/.local/bin/google-chrome", ChromeChannel::Stable});
        linuxPaths.push_back({homeDir + "/.local/bin/chromium", ChromeChannel::Chromium});
    }

    
#ifdef __APPLE__
    auto& pathsToCheck = macosPaths;
#else
    auto& pathsToCheck = linuxPaths;
#endif

    
    for (const auto& [path, channel] : pathsToCheck) {
        try {
            std::filesystem::path p(path);
            if (std::filesystem::exists(p) && std::filesystem::is_regular_file(p)) {
                
                if (access(path.c_str(), X_OK) != 0) {
                    continue;
                }

                ChromeInstallation install;
                install.path = path;
                install.channel = channel;
                install.lastModified = std::filesystem::last_write_time(p);
                install.version = getChromeVersion(path);

                
                bool duplicate = false;
                for (const auto& existing : installations) {
                    try {
                        if (std::filesystem::equivalent(existing.path, path)) {
                            duplicate = true;
                            break;
                        }
                    } catch (...) {}
                }

                if (!duplicate) {
                    installations.push_back(install);
                }
            }
        } catch (const std::filesystem::filesystem_error&) {
            
        }
    }

    
    const char* pathEnv = getenv("PATH");
    if (pathEnv) {
        std::istringstream pathStream(pathEnv);
        std::string pathEntry;
        while (std::getline(pathStream, pathEntry, ':')) {
            if (!pathEntry.empty()) {
                for (const char* name : {"google-chrome", "google-chrome-stable", "chromium", "chromium-browser"}) {
                    std::string fullPath = pathEntry + "/" + name;
                    if (access(fullPath.c_str(), X_OK) == 0) {
                        try {
                            std::filesystem::path p(fullPath);
                            if (std::filesystem::is_regular_file(p)) {
                                bool duplicate = false;
                                for (const auto& existing : installations) {
                                    try {
                                        if (std::filesystem::equivalent(existing.path, fullPath)) {
                                            duplicate = true;
                                            break;
                                        }
                                    } catch (...) {}
                                }
                                if (!duplicate) {
                                    ChromeInstallation install;
                                    install.path = fullPath;
                                    install.channel = (std::string(name).find("chromium") != std::string::npos)
                                        ? ChromeChannel::Chromium : ChromeChannel::Stable;
                                    install.lastModified = std::filesystem::last_write_time(p);
                                    installations.push_back(install);
                                }
                            }
                        } catch (...) {}
                    }
                }
            }
        }
    }

    
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

        
        if (access(path.c_str(), X_OK) != 0) {
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
    
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        return "";
    }

    pid_t pid = fork();
    if (pid == 0) {
        
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        dup2(pipefd[1], STDERR_FILENO);
        close(pipefd[1]);

        execl(path.c_str(), path.c_str(), "--version", nullptr);
        _exit(1);
    } else if (pid > 0) {
        
        close(pipefd[1]);

        char buffer[256];
        std::string output;
        ssize_t bytesRead;
        while ((bytesRead = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0) {
            buffer[bytesRead] = '\0';
            output += buffer;
        }
        close(pipefd[0]);

        int status;
        waitpid(pid, &status, 0);

        
        size_t pos = output.find_first_of("0123456789");
        if (pos != std::string::npos) {
            size_t end = output.find_first_not_of("0123456789.", pos);
            return output.substr(pos, end - pos);
        }
    } else {
        close(pipefd[0]);
        close(pipefd[1]);
    }

    return "";
}


int ChromeLauncher::findFreePort() {
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
                        "allowExtensionsOnCustomDir=true.";
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
    pid_t pid = fork();

    if (pid == 0) {
        

        setsid();

        
        std::vector<char*> argv;
        argv.push_back(const_cast<char*>(chromePath.c_str()));
        for (const auto& arg : args) {
            argv.push_back(const_cast<char*>(arg.c_str()));
        }
        argv.push_back(nullptr);

        
        if (options_.headless) {
            int devnull = open("/dev/null", O_WRONLY);
            if (devnull >= 0) {
                dup2(devnull, STDOUT_FILENO);
                dup2(devnull, STDERR_FILENO);
                close(devnull);
            }
        }

        
        execv(chromePath.c_str(), argv.data());

        
        _exit(1);
    } else if (pid < 0) {
        lastError_ = "Failed to fork process";
        return false;
    }

    
    processId_ = pid;
    return true;
}

bool ChromeLauncher::createTempProfile() {
    
    cleanupStaleTempProfiles(options_.tempProfilePrefix);

    
    const char* tmpDir = getenv("TMPDIR");
    if (!tmpDir) tmpDir = getenv("TMP");
    if (!tmpDir) tmpDir = getenv("TEMP");
    if (!tmpDir) tmpDir = "/tmp";

    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(100000, 999999);

    std::string uniqueFolder = options_.tempProfilePrefix + std::to_string(dis(gen));
    userDataDir_ = std::string(tmpDir) + "/" + uniqueFolder;

    try {
        std::filesystem::create_directories(userDataDir_);
    } catch (const std::filesystem::filesystem_error& e) {
        lastError_ = "Failed to create temp profile directory: " + std::string(e.what());
        return false;
    }

    return true;
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

    const char* tmpDir = getenv("TMPDIR");
    if (!tmpDir) tmpDir = getenv("TMP");
    if (!tmpDir) tmpDir = getenv("TEMP");
    if (!tmpDir) tmpDir = "/tmp";

    try {
        for (const auto& entry : std::filesystem::directory_iterator(tmpDir)) {
            if (!entry.is_directory()) {
                continue;
            }

            std::string folderName = entry.path().filename().string();
            if (folderName.find(prefix) != 0) {
                continue;
            }

            
            try {
                std::filesystem::remove_all(entry.path());
                ++cleanedCount;
            } catch (const std::filesystem::filesystem_error&) {
                
            }
        }
    } catch (const std::filesystem::filesystem_error&) {
        
    }

    return cleanedCount;
}

bool ChromeLauncher::isRunning() const {
    if (processId_ <= 0) return false;

    
    if (::kill(processId_, 0) == 0) {
        return true;
    }

    
    int status;
    pid_t result = waitpid(processId_, &status, WNOHANG);
    return result == 0;  
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
    if (processId_ > 0) {
        
        ::kill(processId_, SIGTERM);

        
        for (int i = 0; i < 50; ++i) {
            int status;
            pid_t result = waitpid(processId_, &status, WNOHANG);
            if (result != 0) {
                break;  
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        
        if (isRunning()) {
            ::kill(processId_, SIGKILL);
            waitpid(processId_, nullptr, 0);
        }

        processId_ = 0;
    }
    launched_ = false;
}

bool ChromeLauncher::waitForExit(int timeoutMs) {
    if (processId_ <= 0) return true;

    if (timeoutMs < 0) {
        
        int status;
        waitpid(processId_, &status, 0);
        return true;
    }

    
    auto startTime = std::chrono::steady_clock::now();
    while (true) {
        int status;
        pid_t result = waitpid(processId_, &status, WNOHANG);
        if (result != 0) {
            return true;  
        }

        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - startTime).count();
        if (elapsed >= timeoutMs) {
            return false;  
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

uint32_t ChromeLauncher::getProcessId() const {
    return static_cast<uint32_t>(processId_);
}

int ChromeLauncher::getExitCode() const {
    if (processId_ <= 0) return -1;

    int status;
    pid_t result = waitpid(processId_, &status, WNOHANG);
    if (result == processId_ && WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }
    return -1;
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
