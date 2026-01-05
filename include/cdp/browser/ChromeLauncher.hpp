

#pragma once


#include <string>
#include <vector>
#include <optional>
#include <functional>
#include <chrono>
#include <filesystem>
#include <atomic>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace cdp {


enum class ChromeChannel {
    Stable,     
    Beta,       
    Dev,        
    Canary,     
    Chromium,   
    Custom      
};


struct ChromeInstallation {
    std::string path;           
    ChromeChannel channel;      
    std::string version;        
    std::filesystem::file_time_type lastModified;  

    bool isValid() const { return !path.empty() && std::filesystem::exists(path); }

    
    std::string channelName() const {
        switch (channel) {
            case ChromeChannel::Stable: return "Stable";
            case ChromeChannel::Beta: return "Beta";
            case ChromeChannel::Dev: return "Dev";
            case ChromeChannel::Canary: return "Canary";
            case ChromeChannel::Chromium: return "Chromium";
            case ChromeChannel::Custom: return "Custom";
            default: return "Unknown";
        }
    }
};


struct ChromeLaunchOptions {
    
    int debuggingPort = 0;              
    std::string host = "127.0.0.1";     

    
    bool useTempProfile = true;         
    std::string userDataDir;            
    std::string tempProfilePrefix = "cdp_chrome_";  

    
    bool headless = false;              
    bool startMaximized = false;        
    int windowWidth = 1280;             
    int windowHeight = 720;             
    int windowX = -1;                   
    int windowY = -1;                   

    
    bool disableGpu = false;            
    bool disableExtensions = true;      
    bool disablePopupBlocking = true;   
    bool disableDefaultApps = true;     
    bool noFirstRun = true;             
    bool noDefaultBrowserCheck = true;  
    bool disableBackgroundNetworking = false;  
    bool disableSync = true;            
    bool disableTranslate = true;       
    bool muteAudio = false;             
    bool ignoreSSLErrors = false;       

    
    std::string proxyServer;            
    std::string proxyBypassList;        

    
    std::vector<std::string> additionalFlags;

    
    ChromeChannel preferredChannel = ChromeChannel::Stable;  
    std::string customChromePath;       

    
    std::string startUrl = "about:blank";  
    int startupWaitMs = 2000;           
    int maxStartupWaitMs = 30000;       

    
    bool cleanupTempProfile = true;     
    bool killOnDestruct = true;         

    
    std::vector<std::string> extensions;

    
    bool extensionIncognitoEnabled = true;

    
    bool extensionFileAccessEnabled = true;

    
    bool allowExtensionsOnCustomDir = false;

    
    std::vector<std::string> buildArgs() const;

    
    static ChromeLaunchOptions headlessMode() {
        ChromeLaunchOptions opts;
        opts.headless = true;
        opts.disableGpu = true;
        return opts;
    }

    static ChromeLaunchOptions automation() {
        ChromeLaunchOptions opts;
        opts.disableExtensions = true;
        opts.disablePopupBlocking = true;
        opts.noFirstRun = true;
        opts.disableSync = true;
        return opts;
    }

    static ChromeLaunchOptions testing() {
        ChromeLaunchOptions opts;
        opts.headless = true;
        opts.disableGpu = true;
        opts.ignoreSSLErrors = true;
        opts.disableExtensions = true;
        opts.noFirstRun = true;
        return opts;
    }

    
    static ChromeLaunchOptions withExtensions(const std::vector<std::string>& extensionPaths) {
        ChromeLaunchOptions opts;
        opts.extensions = extensionPaths;
        opts.disableExtensions = false;  
        opts.disablePopupBlocking = true;
        opts.noFirstRun = true;
        opts.disableSync = true;
        return opts;
    }

    
    static ChromeLaunchOptions headlessWithExtensions(const std::vector<std::string>& extensionPaths) {
        ChromeLaunchOptions opts = withExtensions(extensionPaths);
        opts.headless = true;
        opts.disableGpu = true;
        return opts;
    }
};


class ChromeLauncher {
public:
    ChromeLauncher();
    explicit ChromeLauncher(const ChromeLaunchOptions& options);
    ~ChromeLauncher();

    
    ChromeLauncher(const ChromeLauncher&) = delete;
    ChromeLauncher& operator=(const ChromeLauncher&) = delete;
    ChromeLauncher(ChromeLauncher&& other) noexcept;
    ChromeLauncher& operator=(ChromeLauncher&& other) noexcept;

    
    static std::vector<ChromeInstallation> findAllInstallations();

    
    static std::optional<ChromeInstallation> findBestInstallation();

    
    static std::optional<ChromeInstallation> findInstallation(ChromeChannel channel);

    
    static bool isValidChrome(const std::string& path);

    
    static std::string getChromeVersion(const std::string& path);

    
    static int findFreePort();

    
    static int cleanupStaleTempProfiles(const std::string& prefix = "cdp_chrome_");

    
    [[nodiscard]] bool launch();

    
    [[nodiscard]] bool launch(const ChromeLaunchOptions& options);

    
    [[nodiscard]] bool isRunning() const;

    
    [[nodiscard]] bool waitForReady(int timeoutMs = 30000);

    
    std::string getDebugUrl() const;

    
    std::string getBrowserWebSocketUrl() const;

    
    void kill();

    
    bool waitForExit(int timeoutMs = -1);  

    
    uint32_t getProcessId() const;

    
    int getExitCode() const;

    
    const ChromeInstallation& installation() const { return installation_; }

    
    const std::string& userDataDir() const { return userDataDir_; }

    
    int debuggingPort() const { return options_.debuggingPort; }

    
    const ChromeLaunchOptions& options() const { return options_; }

    
    const std::string& lastError() const { return lastError_; }

private:
    bool startProcess(const std::string& chromePath, const std::vector<std::string>& args);
    bool createTempProfile();
    void cleanupTempProfile();
    bool checkEndpointReady();

    ChromeLaunchOptions options_;
    ChromeInstallation installation_;
    std::string userDataDir_;
    std::string lastError_;
    std::atomic<bool> launched_{false};

#ifdef _WIN32
    HANDLE processHandle_ = nullptr;
    HANDLE threadHandle_ = nullptr;
    DWORD processId_ = 0;
#else
    int processId_ = 0;
#endif
};


std::optional<ChromeLauncher> launchChrome(
    const ChromeLaunchOptions& options = ChromeLaunchOptions{});


std::optional<ChromeLauncher> launchHeadlessChrome(int port = 9222);

} 
