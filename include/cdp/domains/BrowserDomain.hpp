#pragma once

#include "BaseDomain.hpp"
#include <string>
#include <vector>

namespace cdp {


struct WindowBounds {
    int left = 0;
    int top = 0;
    int width = 800;
    int height = 600;
    std::string windowState = "normal";  

    static WindowBounds fromJson(const JsonValue& json);
    JsonValue toJson() const;
};


struct Histogram {
    std::string name;
    int sum = 0;
    int count = 0;
    std::vector<std::pair<int, int>> buckets;  

    static Histogram fromJson(const JsonValue& json);
};


class BrowserDomain : public BaseDomain {
public:
    using BaseDomain::BaseDomain;
    std::string domainName() const override { return "Browser"; }

    
    std::string getVersion();

    
    std::pair<int, WindowBounds> getWindowForTarget(const std::string& targetId = "");
    WindowBounds getWindowBounds(int windowId);
    CDPResponse setWindowBounds(int windowId, const WindowBounds& bounds);

    
    CDPResponse minimizeWindow(int windowId);
    CDPResponse maximizeWindow(int windowId);
    CDPResponse setFullscreen(int windowId, bool fullscreen);
    CDPResponse restoreWindow(int windowId);

    
    CDPResponse close();
    CDPResponse crash();
    CDPResponse crashGpuProcess();

    
    CDPResponse setDownloadBehavior(const std::string& behavior,
                                     const std::string& browserContextId = "",
                                     const std::string& downloadPath = "",
                                     bool eventsEnabled = false);
    
    CDPResponse cancelDownload(const std::string& guid, const std::string& browserContextId = "");

    
    CDPResponse setPermission(const std::string& permission, const std::string& setting,
                               const std::string& origin = "",
                               const std::string& browserContextId = "");
    
    
    CDPResponse grantPermissions(const std::vector<std::string>& permissions,
                                  const std::string& origin = "",
                                  const std::string& browserContextId = "");
    CDPResponse resetPermissions(const std::string& browserContextId = "");

    
    std::vector<Histogram> getHistograms(const std::string& query = "", bool delta = false);
    Histogram getHistogram(const std::string& name, bool delta = false);

    
    std::vector<std::string> getBrowserCommandLine();

    
    CDPResponse setDockTile(const std::string& badgeLabel = "", const std::string& image = "");

    
    CDPResponse executeBrowserCommand(const std::string& commandId);
    

    void onDownloadWillBegin(std::function<void(const std::string& frameId,
                                                 const std::string& guid,
                                                 const std::string& url,
                                                 const std::string& suggestedFilename)> callback);
    void onDownloadProgress(std::function<void(const std::string& guid,
                                                double totalBytes,
                                                double receivedBytes,
                                                const std::string& state)> callback);
};

} 
