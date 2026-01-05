#pragma once

#include "BaseDomain.hpp"
#include <vector>
#include <optional>

namespace cdp {


struct FrameInfo {
    std::string id;
    std::string parentId;
    std::string loaderId;
    std::string name;
    std::string url;
    std::string securityOrigin;
    std::string mimeType;

    static FrameInfo fromJson(const JsonValue& json);
};


enum class ScreenshotFormat {
    Jpeg,
    Png,
    Webp
};


struct Viewport {
    double x = 0;
    double y = 0;
    double width = 800;
    double height = 600;
    double scale = 1;

    JsonValue toJson() const;
};


struct NavigationResult {
    std::string frameId;
    std::string loaderId;
    std::string errorText;

    bool success() const { return errorText.empty(); }
};


class PageDomain : public BaseDomain {
public:
    using BaseDomain::BaseDomain;
    std::string domainName() const override { return "Page"; }

    
    NavigationResult navigate(const std::string& url);
    CDPResponse navigateToHistoryEntry(int entryId);
    CDPResponse reload(bool ignoreCache = false, const std::string& scriptToEvaluateOnLoad = "");
    CDPResponse stopLoading();
    CDPResponse goBack();
    CDPResponse goForward();

    
    struct NavigationEntry {
        int id;
        std::string url;
        std::string title;
        std::string transitionType;
    };
    std::pair<int, std::vector<NavigationEntry>> getNavigationHistory();

    
    FrameInfo getFrameTree();
    std::vector<FrameInfo> getAllFrames();

    
    std::string getResourceContent(const std::string& frameId, const std::string& url);
    CDPResponse setDocumentContent(const std::string& frameId, const std::string& html);

    
    std::string captureScreenshot(ScreenshotFormat format = ScreenshotFormat::Png,
                                   int quality = 100,
                                   std::optional<Viewport> clip = std::nullopt,
                                   bool fullPage = false);
    std::string printToPDF(bool landscape = false,
                           bool displayHeaderFooter = false,
                           bool printBackground = true,
                           double scale = 1.0,
                           double paperWidth = 8.5,
                           double paperHeight = 11.0,
                           double marginTop = 0.4,
                           double marginBottom = 0.4,
                           double marginLeft = 0.4,
                           double marginRight = 0.4);

    
    CDPResponse handleJavaScriptDialog(bool accept, const std::string& promptText = "");

    
    CDPResponse bringToFront();
    CDPResponse close();
    CDPResponse crash();

    
    CDPResponse addScriptToEvaluateOnLoad(const std::string& scriptSource);
    CDPResponse addScriptToEvaluateOnNewDocument(const std::string& source,
                                                  const std::string& worldName = "");
    CDPResponse removeScriptToEvaluateOnNewDocument(const std::string& identifier);

    
    CDPResponse setPermissionOverride(const std::string& origin,
                                       const std::string& permission,
                                       const std::string& setting);

    
    void onLoadEventFired(std::function<void(double timestamp)> callback);
    void onDomContentEventFired(std::function<void(double timestamp)> callback);
    void onFrameNavigated(std::function<void(const FrameInfo&)> callback);
    void onFrameStartedLoading(std::function<void(const std::string& frameId)> callback);
    void onFrameStoppedLoading(std::function<void(const std::string& frameId)> callback);
    void onJavascriptDialogOpening(std::function<void(const std::string& url,
                                                       const std::string& message,
                                                       const std::string& type,
                                                       bool hasBrowserHandler,
                                                       const std::string& defaultPrompt)> callback);
    void onJavascriptDialogClosed(std::function<void(bool result, const std::string& userInput)> callback);
    void onWindowOpen(std::function<void(const std::string& url,
                                          const std::string& windowName,
                                          const std::vector<std::string>& windowFeatures,
                                          bool userGesture)> callback);
};

} 
