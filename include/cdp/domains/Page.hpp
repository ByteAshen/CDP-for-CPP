#pragma once

#include "Domain.hpp"
#include "../core/Enums.hpp"
#include <optional>

namespace cdp {


struct Viewport {
    double x = 0;
    double y = 0;
    double width = 800;
    double height = 600;
    double scale = 1;

    JsonValue toJson() const {
        JsonObject obj;
        obj["x"] = x;
        obj["y"] = y;
        obj["width"] = width;
        obj["height"] = height;
        obj["scale"] = scale;
        return obj;
    }
};


struct FontFamilies {
    std::string standard;
    std::string fixed;
    std::string serif;
    std::string sansSerif;
    std::string cursive;
    std::string fantasy;
    std::string math;

    JsonValue toJson() const {
        JsonObject obj;
        if (!standard.empty()) obj["standard"] = standard;
        if (!fixed.empty()) obj["fixed"] = fixed;
        if (!serif.empty()) obj["serif"] = serif;
        if (!sansSerif.empty()) obj["sansSerif"] = sansSerif;
        if (!cursive.empty()) obj["cursive"] = cursive;
        if (!fantasy.empty()) obj["fantasy"] = fantasy;
        if (!math.empty()) obj["math"] = math;
        return obj;
    }
};


struct FontSizes {
    int standard = 0;
    int fixed = 0;

    JsonValue toJson() const {
        JsonObject obj;
        if (standard > 0) obj["standard"] = standard;
        if (fixed > 0) obj["fixed"] = fixed;
        return obj;
    }
};


struct CompilationCacheParams {
    std::string url;
    bool eager = false;

    JsonValue toJson() const {
        JsonObject obj;
        obj["url"] = url;
        if (eager) obj["eager"] = true;
        return obj;
    }
};


class Page : public Domain {
public:
    explicit Page(CDPConnection& connection) : Domain(connection, "Page") {}

    
    CDPResponse navigate(const std::string& url,
                          const std::string& referrer = "",
                          const std::string& transitionType = "",
                          const std::string& frameId = "",
                          const std::string& referrerPolicy = "") {
        Params params;
        params.set("url", url);
        if (!referrer.empty()) params.set("referrer", referrer);
        if (!transitionType.empty()) params.set("transitionType", transitionType);
        if (!frameId.empty()) params.set("frameId", frameId);
        
        if (!referrerPolicy.empty()) params.set("referrerPolicy", referrerPolicy);
        return call("navigate", params);
    }

    
    CDPResponse navigate(const std::string& url,
                          std::optional<TransitionType> transitionType,
                          std::optional<ReferrerPolicy> referrerPolicy = std::nullopt,
                          const std::string& referrer = "",
                          const std::string& frameId = "") {
        Params params;
        params.set("url", url);
        if (!referrer.empty()) params.set("referrer", referrer);
        if (transitionType) params.set("transitionType", std::string(toString(*transitionType)));
        if (!frameId.empty()) params.set("frameId", frameId);
        if (referrerPolicy) params.set("referrerPolicy", std::string(toString(*referrerPolicy)));
        return call("navigate", params);
    }

    
    CDPResponse navigateAndWaitForLoad(const std::string& url, int timeoutMs = 30000) {
        auto loadFuture = once("loadEventFired");
        auto result = navigate(url);
        if (result.hasError) return result;
        auto status = loadFuture.wait_for(std::chrono::milliseconds(timeoutMs));
        if (status != std::future_status::ready) {
            result.hasError = true;
            result.errorMessage = "Navigation timeout waiting for load event";
        }
        return result;
    }

    
    std::string getCurrentUrl() {
        auto result = getFrameTree();
        if (result.hasError) return "";
        return result.result.getStringAt("frameTree/frame/url", "");
    }

    
    std::string getTitle() {
        auto result = connection_.sendCommandSync("Runtime.evaluate",
            Params().set("expression", "document.title").set("returnByValue", true).build());
        if (result.hasError) return "";
        return result.result.getStringAt("result/value", "");
    }

    CDPResponse navigateToHistoryEntry(int entryId) {
        return call("navigateToHistoryEntry", Params().set("entryId", entryId));
    }

    CDPResponse reload(bool ignoreCache = false, const std::string& scriptToEvaluateOnLoad = "") {
        Params params;
        if (ignoreCache) params.set("ignoreCache", true);
        if (!scriptToEvaluateOnLoad.empty()) params.set("scriptToEvaluateOnLoad", scriptToEvaluateOnLoad);
        return call("reload", params);
    }

    CDPResponse stopLoading() { return call("stopLoading"); }

    CDPResponse getNavigationHistory() { return call("getNavigationHistory"); }

    
    CDPResponse getFrameTree() { return call("getFrameTree"); }

    
    CDPResponse getResourceContent(const std::string& frameId, const std::string& url) {
        return call("getResourceContent", Params()
            .set("frameId", frameId)
            .set("url", url));
    }

    CDPResponse setDocumentContent(const std::string& frameId, const std::string& html) {
        return call("setDocumentContent", Params()
            .set("frameId", frameId)
            .set("html", html));
    }

    
    CDPResponse captureScreenshot(const std::string& format = "png",
                                   int quality = 100,
                                   const Viewport* clip = nullptr,
                                   bool fromSurface = true,
                                   bool captureBeyondViewport = false,
                                   bool optimizeForSpeed = true,
                                   int timeoutMs = 60000) {
        Params params;
        params.set("format", format);
        if (format == "jpeg" || format == "webp") params.set("quality", quality);
        if (clip) params.set("clip", clip->toJson());
        params.set("fromSurface", fromSurface);
        if (captureBeyondViewport) params.set("captureBeyondViewport", true);
        if (optimizeForSpeed) params.set("optimizeForSpeed", true);
        return call("captureScreenshot", params, timeoutMs);
    }

    
    CDPResponse captureScreenshot(ImageFormat format,
                                   int quality = 100,
                                   const Viewport* clip = nullptr,
                                   bool fromSurface = true,
                                   bool captureBeyondViewport = false,
                                   bool optimizeForSpeed = true,
                                   int timeoutMs = 60000) {
        Params params;
        params.set("format", std::string(toString(format)));
        if (format == ImageFormat::Jpeg || format == ImageFormat::Webp) {
            params.set("quality", quality);
        }
        if (clip) params.set("clip", clip->toJson());
        params.set("fromSurface", fromSurface);
        if (captureBeyondViewport) params.set("captureBeyondViewport", true);
        if (optimizeForSpeed) params.set("optimizeForSpeed", true);
        return call("captureScreenshot", params, timeoutMs);
    }

    
    CDPResponse printToPDF(bool landscape = false,
                           bool displayHeaderFooter = false,
                           bool printBackground = false,
                           double scale = 1.0,
                           double paperWidth = 8.5,
                           double paperHeight = 11.0,
                           double marginTop = 0.4,
                           double marginBottom = 0.4,
                           double marginLeft = 0.4,
                           double marginRight = 0.4,
                           const std::string& pageRanges = "",
                           const std::string& headerTemplate = "",
                           const std::string& footerTemplate = "",
                           bool preferCSSPageSize = false,
                           const std::string& transferMode = "ReturnAsBase64",
                           bool generateTaggedPDF = false,
                           bool generateDocumentOutline = false) {
        Params params;
        params.set("landscape", landscape);
        params.set("displayHeaderFooter", displayHeaderFooter);
        params.set("printBackground", printBackground);
        params.set("scale", scale);
        params.set("paperWidth", paperWidth);
        params.set("paperHeight", paperHeight);
        params.set("marginTop", marginTop);
        params.set("marginBottom", marginBottom);
        params.set("marginLeft", marginLeft);
        params.set("marginRight", marginRight);
        if (!pageRanges.empty()) params.set("pageRanges", pageRanges);
        if (!headerTemplate.empty()) params.set("headerTemplate", headerTemplate);
        if (!footerTemplate.empty()) params.set("footerTemplate", footerTemplate);
        params.set("preferCSSPageSize", preferCSSPageSize);
        params.set("transferMode", transferMode);
        if (generateTaggedPDF) params.set("generateTaggedPDF", true);
        if (generateDocumentOutline) params.set("generateDocumentOutline", true);
        return call("printToPDF", params);
    }

    
    CDPResponse handleJavaScriptDialog(bool accept, const std::string& promptText = "") {
        Params params;
        params.set("accept", accept);
        if (!promptText.empty()) params.set("promptText", promptText);
        return call("handleJavaScriptDialog", params);
    }

    
    CDPResponse bringToFront() { return call("bringToFront"); }
    CDPResponse close() { return call("close"); }
    CDPResponse crash() { return call("crash"); }

    
    CDPResponse addScriptToEvaluateOnLoad(const std::string& scriptSource) {
        return call("addScriptToEvaluateOnLoad", Params().set("scriptSource", scriptSource));
    }

    CDPResponse addScriptToEvaluateOnNewDocument(const std::string& source,
                                                  const std::string& worldName = "",
                                                  bool includeCommandLineAPI = false,
                                                  bool runImmediately = false) {
        Params params;
        params.set("source", source);
        if (!worldName.empty()) params.set("worldName", worldName);
        if (includeCommandLineAPI) params.set("includeCommandLineAPI", true);
        if (runImmediately) params.set("runImmediately", true);
        return call("addScriptToEvaluateOnNewDocument", params);
    }

    CDPResponse removeScriptToEvaluateOnNewDocument(const std::string& identifier) {
        return call("removeScriptToEvaluateOnNewDocument", Params().set("identifier", identifier));
    }

    
    CDPResponse startScreencast(const std::string& format = "jpeg",
                                 int quality = 80,
                                 int maxWidth = 0,
                                 int maxHeight = 0,
                                 int everyNthFrame = 1) {
        Params params;
        params.set("format", format);
        params.set("quality", quality);
        if (maxWidth > 0) params.set("maxWidth", maxWidth);
        if (maxHeight > 0) params.set("maxHeight", maxHeight);
        params.set("everyNthFrame", everyNthFrame);
        return call("startScreencast", params);
    }

    CDPResponse stopScreencast() { return call("stopScreencast"); }

    CDPResponse screencastFrameAck(int sessionId) {
        return call("screencastFrameAck", Params().set("sessionId", sessionId));
    }

    
    CDPResponse setInterceptFileChooserDialog(bool enabled) {
        return call("setInterceptFileChooserDialog", Params().set("enabled", enabled));
    }

    
    CDPResponse setLifecycleEventsEnabled(bool enabled) {
        return call("setLifecycleEventsEnabled", Params().set("enabled", enabled));
    }

    CDPResponse generateTestReport(const std::string& message, const std::string& group = "") {
        Params params;
        params.set("message", message);
        if (!group.empty()) params.set("group", group);
        return call("generateTestReport", params);
    }

    
    CDPResponse createIsolatedWorld(const std::string& frameId,
                                     const std::string& worldName = "",
                                     bool grantUniversalAccess = false) {
        Params params;
        params.set("frameId", frameId);
        if (!worldName.empty()) params.set("worldName", worldName);
        if (grantUniversalAccess) params.set("grantUniveralAccess", true);
        return call("createIsolatedWorld", params);
    }

    CDPResponse getAppManifest(const std::string& manifestId = "") {
        Params params;
        if (!manifestId.empty()) params.set("manifestId", manifestId);
        return call("getAppManifest", params);
    }

    CDPResponse getLayoutMetrics() { return call("getLayoutMetrics"); }

    CDPResponse resetNavigationHistory() { return call("resetNavigationHistory"); }

    CDPResponse setBypassCSP(bool enabled) {
        return call("setBypassCSP", Params().set("enabled", enabled));
    }

    CDPResponse getInstallabilityErrors() { return call("getInstallabilityErrors"); }

    CDPResponse getOriginTrials(const std::string& frameId) {
        return call("getOriginTrials", Params().set("frameId", frameId));
    }

    CDPResponse getPermissionsPolicyState(const std::string& frameId) {
        return call("getPermissionsPolicyState", Params().set("frameId", frameId));
    }

    CDPResponse getResourceTree() { return call("getResourceTree"); }

    CDPResponse searchInResource(const std::string& frameId,
                                  const std::string& url,
                                  const std::string& query,
                                  bool caseSensitive = false,
                                  bool isRegex = false) {
        Params params;
        params.set("frameId", frameId);
        params.set("url", url);
        params.set("query", query);
        if (caseSensitive) params.set("caseSensitive", true);
        if (isRegex) params.set("isRegex", true);
        return call("searchInResource", params);
    }

    CDPResponse setAdBlockingEnabled(bool enabled) {
        return call("setAdBlockingEnabled", Params().set("enabled", enabled));
    }

    CDPResponse waitForDebugger() { return call("waitForDebugger"); }

    
    CDPResponse enable(bool enableFileChooserOpenedEvent = false) {
        Params params;
        if (enableFileChooserOpenedEvent) params.set("enableFileChooserOpenedEvent", true);
        return call("enable", params);
    }

    CDPResponse disable() { return call("disable"); }

    
    CDPResponse captureSnapshot(const std::string& format = "mhtml") {
        return call("captureSnapshot", Params().set("format", format));
    }

    
    CDPResponse getAppId() { return call("getAppId"); }

    
    CDPResponse getManifestIcons() { return call("getManifestIcons"); }

    
    CDPResponse getAdScriptAncestry(const std::string& frameId) {
        return call("getAdScriptAncestry", Params().set("frameId", frameId));
    }

    
    CDPResponse getAnnotatedPageContent(bool includeActionableInformation = true) {
        Params params;
        if (!includeActionableInformation) params.set("includeActionableInformation", false);
        return call("getAnnotatedPageContent", params);
    }

    
    CDPResponse setFontFamilies(const FontFamilies& fontFamilies) {
        return call("setFontFamilies", Params().set("fontFamilies", fontFamilies.toJson()));
    }

    
    CDPResponse setFontSizes(const FontSizes& fontSizes) {
        return call("setFontSizes", Params().set("fontSizes", fontSizes.toJson()));
    }

    
    CDPResponse setDownloadBehavior(const std::string& behavior, const std::string& downloadPath = "") {
        Params params;
        params.set("behavior", behavior);  
        if (!downloadPath.empty()) params.set("downloadPath", downloadPath);
        return call("setDownloadBehavior", params);
    }

    
    CDPResponse deleteCookie(const std::string& cookieName, const std::string& url) {
        return call("deleteCookie", Params().set("cookieName", cookieName).set("url", url));
    }

    
    CDPResponse produceCompilationCache(const std::vector<CompilationCacheParams>& scripts) {
        JsonArray arr;
        for (const auto& s : scripts) arr.push_back(s.toJson());
        return call("produceCompilationCache", Params().set("scripts", arr));
    }

    CDPResponse addCompilationCache(const std::string& url, const std::string& data) {
        return call("addCompilationCache", Params().set("url", url).set("data", data));
    }

    CDPResponse clearCompilationCache() { return call("clearCompilationCache"); }

    
    CDPResponse setDeviceMetricsOverride(int width, int height, double deviceScaleFactor, bool mobile,
                                          double scale = 0, int screenWidth = 0, int screenHeight = 0,
                                          int positionX = 0, int positionY = 0) {
        Params params;
        params.set("width", width);
        params.set("height", height);
        params.set("deviceScaleFactor", deviceScaleFactor);
        params.set("mobile", mobile);
        if (scale > 0) params.set("scale", scale);
        if (screenWidth > 0) params.set("screenWidth", screenWidth);
        if (screenHeight > 0) params.set("screenHeight", screenHeight);
        if (positionX > 0) params.set("positionX", positionX);
        if (positionY > 0) params.set("positionY", positionY);
        return call("setDeviceMetricsOverride", params);
    }

    CDPResponse clearDeviceMetricsOverride() { return call("clearDeviceMetricsOverride"); }

    
    CDPResponse setDeviceOrientationOverride(double alpha, double beta, double gamma) {
        return call("setDeviceOrientationOverride",
                   Params().set("alpha", alpha).set("beta", beta).set("gamma", gamma));
    }

    CDPResponse clearDeviceOrientationOverride() { return call("clearDeviceOrientationOverride"); }

    
    CDPResponse setGeolocationOverride(double latitude = 0, double longitude = 0, double accuracy = 0) {
        Params params;
        if (latitude != 0) params.set("latitude", latitude);
        if (longitude != 0) params.set("longitude", longitude);
        if (accuracy != 0) params.set("accuracy", accuracy);
        return call("setGeolocationOverride", params);
    }

    CDPResponse clearGeolocationOverride() { return call("clearGeolocationOverride"); }

    
    CDPResponse setTouchEmulationEnabled(bool enabled, const std::string& configuration = "") {
        Params params;
        params.set("enabled", enabled);
        if (!configuration.empty()) params.set("configuration", configuration);  
        return call("setTouchEmulationEnabled", params);
    }

    
    CDPResponse setWebLifecycleState(const std::string& state) {
        return call("setWebLifecycleState", Params().set("state", state));  
    }

    
    CDPResponse setSPCTransactionMode(const std::string& mode) {
        
        return call("setSPCTransactionMode", Params().set("mode", mode));
    }

    
    CDPResponse setRPHRegistrationMode(const std::string& mode) {
        
        return call("setRPHRegistrationMode", Params().set("mode", mode));
    }

    
    CDPResponse setPrerenderingAllowed(bool isAllowed) {
        return call("setPrerenderingAllowed", Params().set("isAllowed", isAllowed));
    }

    
    void onLoadEventFired(std::function<void(double timestamp)> callback) {
        on("loadEventFired", [callback](const CDPEvent& event) {
            callback(event.params["timestamp"].getNumber());
        });
    }

    void onDomContentEventFired(std::function<void(double timestamp)> callback) {
        on("domContentEventFired", [callback](const CDPEvent& event) {
            callback(event.params["timestamp"].getNumber());
        });
    }

    void onFrameNavigated(std::function<void(const JsonValue& frame)> callback) {
        on("frameNavigated", [callback](const CDPEvent& event) {
            callback(event.params["frame"]);
        });
    }

    void onFrameStartedLoading(std::function<void(const std::string& frameId)> callback) {
        on("frameStartedLoading", [callback](const CDPEvent& event) {
            callback(event.params["frameId"].getString());
        });
    }

    void onFrameStoppedLoading(std::function<void(const std::string& frameId)> callback) {
        on("frameStoppedLoading", [callback](const CDPEvent& event) {
            callback(event.params["frameId"].getString());
        });
    }

    void onJavascriptDialogOpening(std::function<void(const std::string& url,
                                                       const std::string& message,
                                                       const std::string& type,
                                                       bool hasBrowserHandler,
                                                       const std::string& defaultPrompt)> callback) {
        on("javascriptDialogOpening", [callback](const CDPEvent& event) {
            callback(
                event.params["url"].getString(),
                event.params["message"].getString(),
                event.params["type"].getString(),
                event.params["hasBrowserHandler"].getBool(),
                event.params["defaultPrompt"].getString()
            );
        });
    }

    void onLifecycleEvent(std::function<void(const std::string& frameId,
                                              const std::string& loaderId,
                                              const std::string& name,
                                              double timestamp)> callback) {
        on("lifecycleEvent", [callback](const CDPEvent& event) {
            callback(
                event.params["frameId"].getString(),
                event.params["loaderId"].getString(),
                event.params["name"].getString(),
                event.params["timestamp"].getNumber()
            );
        });
    }

    void onScreencastFrame(std::function<void(const std::string& data,
                                               const JsonValue& metadata,
                                               int sessionId)> callback) {
        on("screencastFrame", [callback](const CDPEvent& event) {
            callback(
                event.params["data"].getString(),
                event.params["metadata"],
                event.params["sessionId"].getInt()
            );
        });
    }

    void onFileChooserOpened(std::function<void(const std::string& frameId,
                                                 const std::string& backendNodeId,
                                                 const std::string& mode)> callback) {
        on("fileChooserOpened", [callback](const CDPEvent& event) {
            callback(
                event.params["frameId"].getString(),
                event.params["backendNodeId"].getString(),
                event.params["mode"].getString()
            );
        });
    }

    
    void onFrameAttached(std::function<void(const std::string& frameId,
                                             const std::string& parentFrameId,
                                             const JsonValue& stack)> callback) {
        on("frameAttached", [callback](const CDPEvent& event) {
            callback(
                event.params["frameId"].getString(),
                event.params["parentFrameId"].getString(),
                event.params["stack"]
            );
        });
    }

    
    void onFrameDetached(std::function<void(const std::string& frameId,
                                             const std::string& reason)> callback) {
        on("frameDetached", [callback](const CDPEvent& event) {
            callback(
                event.params["frameId"].getString(),
                event.params["reason"].getString()
            );
        });
    }

    
    void onDocumentOpened(std::function<void(const JsonValue& frame)> callback) {
        on("documentOpened", [callback](const CDPEvent& event) {
            callback(event.params["frame"]);
        });
    }

    
    void onFrameRequestedNavigation(std::function<void(const std::string& frameId,
                                                        const std::string& url,
                                                        const std::string& reason,
                                                        const std::string& disposition)> callback) {
        on("frameRequestedNavigation", [callback](const CDPEvent& event) {
            callback(
                event.params["frameId"].getString(),
                event.params["url"].getString(),
                event.params["reason"].getString(),
                event.params["disposition"].getString()
            );
        });
    }

    
    void onNavigatedWithinDocument(std::function<void(const std::string& frameId,
                                                       const std::string& url,
                                                       const std::string& navigationType)> callback) {
        on("navigatedWithinDocument", [callback](const CDPEvent& event) {
            callback(
                event.params["frameId"].getString(),
                event.params["url"].getString(),
                event.params["navigationType"].getString()
            );
        });
    }

    
    void onFrameScheduledNavigation(std::function<void(const std::string& frameId,
                                                        double delay,
                                                        const std::string& reason,
                                                        const std::string& url)> callback) {
        on("frameScheduledNavigation", [callback](const CDPEvent& event) {
            callback(
                event.params["frameId"].getString(),
                event.params["delay"].getNumber(),
                event.params["reason"].getString(),
                event.params["url"].getString()
            );
        });
    }

    
    void onFrameClearedScheduledNavigation(std::function<void(const std::string& frameId)> callback) {
        on("frameClearedScheduledNavigation", [callback](const CDPEvent& event) {
            callback(event.params["frameId"].getString());
        });
    }

    
    void onInterstitialShown(std::function<void()> callback) {
        on("interstitialShown", [callback](const CDPEvent&) {
            callback();
        });
    }

    
    void onInterstitialHidden(std::function<void()> callback) {
        on("interstitialHidden", [callback](const CDPEvent&) {
            callback();
        });
    }

    
    void onBackForwardCacheNotUsed(std::function<void(const std::string& loaderId,
                                                       const std::string& frameId,
                                                       const JsonValue& notRestoredExplanations)> callback) {
        on("backForwardCacheNotUsed", [callback](const CDPEvent& event) {
            callback(
                event.params["loaderId"].getString(),
                event.params["frameId"].getString(),
                event.params["notRestoredExplanations"]
            );
        });
    }
};

} 
