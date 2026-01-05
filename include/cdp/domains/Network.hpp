#pragma once

#include "Domain.hpp"

namespace cdp {


struct CookiePartitionKey {
    std::string topLevelSite;
    bool hasCrossSiteAncestor = false;

    JsonValue toJson() const {
        JsonObject obj;
        obj["topLevelSite"] = topLevelSite;
        if (hasCrossSiteAncestor) obj["hasCrossSiteAncestor"] = true;
        return obj;
    }
};


struct CookieParam {
    std::string name;
    std::string value;
    std::string url;
    std::string domain;
    std::string path = "/";
    bool secure = false;
    bool httpOnly = false;
    std::string sameSite;  
    double expires = -1;
    std::string priority;  
    bool sameParty = false;
    std::string sourceScheme;  
    int sourcePort = -1;
    CookiePartitionKey* partitionKey = nullptr;

    JsonValue toJson() const {
        JsonObject obj;
        obj["name"] = name;
        obj["value"] = value;
        if (!url.empty()) obj["url"] = url;
        if (!domain.empty()) obj["domain"] = domain;
        if (!path.empty()) obj["path"] = path;
        if (secure) obj["secure"] = true;
        if (httpOnly) obj["httpOnly"] = true;
        if (!sameSite.empty()) obj["sameSite"] = sameSite;
        if (expires >= 0) obj["expires"] = expires;
        if (!priority.empty()) obj["priority"] = priority;
        if (sameParty) obj["sameParty"] = true;
        if (!sourceScheme.empty()) obj["sourceScheme"] = sourceScheme;
        if (sourcePort >= 0) obj["sourcePort"] = sourcePort;
        if (partitionKey) obj["partitionKey"] = partitionKey->toJson();
        return obj;
    }
};


struct NetworkConditions {
    bool offline = false;
    double latency = 0;  
    double downloadThroughput = -1;  
    double uploadThroughput = -1;
    std::string connectionType;  
};


class Network : public Domain {
public:
    explicit Network(CDPConnection& connection) : Domain(connection, "Network") {}

    
    CDPResponse clearBrowserCache() { return call("clearBrowserCache"); }
    CDPResponse clearBrowserCookies() { return call("clearBrowserCookies"); }

    CDPResponse setCacheDisabled(bool cacheDisabled) {
        return call("setCacheDisabled", Params().set("cacheDisabled", cacheDisabled));
    }

    
    CDPResponse getCookies(const std::vector<std::string>& urls = {}) {
        Params params;
        if (!urls.empty()) {
            JsonArray urlsArray;
            for (const auto& u : urls) urlsArray.push_back(u);
            params.set("urls", urlsArray);
        }
        return call("getCookies", params);
    }

    CDPResponse getAllCookies() { return call("getAllCookies"); }

    CDPResponse setCookie(const CookieParam& cookie) {
        return call("setCookie", Params()
            .set("name", cookie.name)
            .set("value", cookie.value)
            .set("url", cookie.url)
            .set("domain", cookie.domain)
            .set("path", cookie.path)
            .set("secure", cookie.secure)
            .set("httpOnly", cookie.httpOnly)
            .set("sameSite", cookie.sameSite)
            .set("expires", cookie.expires));
    }

    
    CDPResponse setCookie(const std::string& name, const std::string& value,
                          const std::string& domain, const std::string& path = "/") {
        CookieParam cookie;
        cookie.name = name;
        cookie.value = value;
        cookie.domain = domain;
        cookie.path = path;
        return setCookie(cookie);
    }

    
    CDPResponse setCookieForUrl(const std::string& name, const std::string& value,
                                 const std::string& url) {
        CookieParam cookie;
        cookie.name = name;
        cookie.value = value;
        cookie.url = url;
        return setCookie(cookie);
    }

    CDPResponse setCookies(const std::vector<CookieParam>& cookies) {
        JsonArray cookiesArray;
        for (const auto& c : cookies) cookiesArray.push_back(c.toJson());
        return call("setCookies", Params().set("cookies", cookiesArray));
    }

    CDPResponse deleteCookies(const std::string& name,
                               const std::string& url = "",
                               const std::string& domain = "",
                               const std::string& path = "",
                               const CookiePartitionKey* partitionKey = nullptr) {
        Params params;
        params.set("name", name);
        if (!url.empty()) params.set("url", url);
        if (!domain.empty()) params.set("domain", domain);
        if (!path.empty()) params.set("path", path);
        if (partitionKey) params.set("partitionKey", partitionKey->toJson());
        return call("deleteCookies", params);
    }

    
    CDPResponse getResponseBody(const std::string& requestId) {
        return call("getResponseBody", Params().set("requestId", requestId));
    }

    CDPResponse getResponseBodyForInterception(const std::string& interceptionId) {
        return call("getResponseBodyForInterception", Params().set("interceptionId", interceptionId));
    }

    CDPResponse takeResponseBodyForInterceptionAsStream(const std::string& interceptionId) {
        return call("takeResponseBodyForInterceptionAsStream", Params().set("interceptionId", interceptionId));
    }

    
    CDPResponse getRequestPostData(const std::string& requestId) {
        return call("getRequestPostData", Params().set("requestId", requestId));
    }

    
    CDPResponse emulateNetworkConditions(bool offline,
                                          double latency,
                                          double downloadThroughput,
                                          double uploadThroughput,
                                          const std::string& connectionType = "") {
        Params params;
        params.set("offline", offline);
        params.set("latency", latency);
        params.set("downloadThroughput", downloadThroughput);
        params.set("uploadThroughput", uploadThroughput);
        if (!connectionType.empty()) params.set("connectionType", connectionType);
        return call("emulateNetworkConditions", params);
    }

    CDPResponse emulateNetworkConditions(const NetworkConditions& conditions) {
        return emulateNetworkConditions(
            conditions.offline,
            conditions.latency,
            conditions.downloadThroughput,
            conditions.uploadThroughput,
            conditions.connectionType
        );
    }

    
    CDPResponse setUserAgentOverride(const std::string& userAgent,
                                      const std::string& acceptLanguage = "",
                                      const std::string& platform = "",
                                      const JsonValue& userAgentMetadata = JsonValue()) {
        Params params;
        params.set("userAgent", userAgent);
        if (!acceptLanguage.empty()) params.set("acceptLanguage", acceptLanguage);
        if (!platform.empty()) params.set("platform", platform);
        if (!userAgentMetadata.isNull()) params.set("userAgentMetadata", userAgentMetadata);
        return call("setUserAgentOverride", params);
    }

    
    CDPResponse setExtraHTTPHeaders(const std::map<std::string, std::string>& headers) {
        JsonObject headersObj;
        for (const auto& [key, value] : headers) {
            headersObj[key] = value;
        }
        return call("setExtraHTTPHeaders", Params().set("headers", headersObj));
    }

    
    CDPResponse setBlockedURLs(const std::vector<std::string>& urls) {
        JsonArray urlsArray;
        for (const auto& u : urls) urlsArray.push_back(u);
        return call("setBlockedURLs", Params().set("urls", urlsArray));
    }

    
    CDPResponse setBypassServiceWorker(bool bypass) {
        return call("setBypassServiceWorker", Params().set("bypass", bypass));
    }

    
    CDPResponse setDataSizeLimitsForTest(int maxTotalSize, int maxResourceSize) {
        return call("setDataSizeLimitsForTest", Params()
            .set("maxTotalSize", maxTotalSize)
            .set("maxResourceSize", maxResourceSize));
    }

    
    CDPResponse setAcceptedEncodings(const std::vector<std::string>& encodings) {
        JsonArray arr;
        for (const auto& e : encodings) arr.push_back(e);
        return call("setAcceptedEncodings", Params().set("encodings", arr));
    }

    CDPResponse clearAcceptedEncodingsOverride() { return call("clearAcceptedEncodingsOverride"); }

    
    CDPResponse searchInResponseBody(const std::string& requestId,
                                      const std::string& query,
                                      bool caseSensitive = false,
                                      bool isRegex = false) {
        Params params;
        params.set("requestId", requestId);
        params.set("query", query);
        if (caseSensitive) params.set("caseSensitive", true);
        if (isRegex) params.set("isRegex", true);
        return call("searchInResponseBody", params);
    }

    
    CDPResponse enable(int maxTotalBufferSize = 0,
                        int maxResourceBufferSize = 0,
                        int maxPostDataSize = 0,
                        bool reportDirectSocketTraffic = false,
                        bool enableDurableMessages = false) {
        Params params;
        if (maxTotalBufferSize > 0) params.set("maxTotalBufferSize", maxTotalBufferSize);
        if (maxResourceBufferSize > 0) params.set("maxResourceBufferSize", maxResourceBufferSize);
        if (maxPostDataSize > 0) params.set("maxPostDataSize", maxPostDataSize);
        if (reportDirectSocketTraffic) params.set("reportDirectSocketTraffic", true);
        if (enableDurableMessages) params.set("enableDurableMessages", true);
        return call("enable", params);
    }

    
    CDPResponse getCertificate(const std::string& origin) {
        return call("getCertificate", Params().set("origin", origin));
    }

    
    CDPResponse loadNetworkResource(const std::string& url,
                                     const std::string& frameId = "",
                                     const JsonValue& options = JsonValue()) {
        Params params;
        params.set("url", url);
        if (!frameId.empty()) params.set("frameId", frameId);
        if (!options.isNull()) params.set("options", options);
        return call("loadNetworkResource", params);
    }

    
    CDPResponse streamResourceContent(const std::string& requestId) {
        return call("streamResourceContent", Params().set("requestId", requestId));
    }

    
    CDPResponse setAttachDebugStack(bool enabled) {
        return call("setAttachDebugStack", Params().set("enabled", enabled));
    }

    
    CDPResponse enableReportingApi(bool enable) {
        return call("enableReportingApi", Params().set("enable", enable));
    }

    
    CDPResponse getSecurityIsolationStatus(const std::string& frameId = "") {
        Params params;
        if (!frameId.empty()) params.set("frameId", frameId);
        return call("getSecurityIsolationStatus", params);
    }

    
    CDPResponse replayXHR(const std::string& requestId) {
        return call("replayXHR", Params().set("requestId", requestId));
    }

    
    CDPResponse disable() { return call("disable"); }

    
    CDPResponse canClearBrowserCache() { return call("canClearBrowserCache"); }

    
    CDPResponse canClearBrowserCookies() { return call("canClearBrowserCookies"); }

    
    CDPResponse canEmulateNetworkConditions() { return call("canEmulateNetworkConditions"); }

    
    CDPResponse setRequestInterception(const JsonArray& patterns) {
        return call("setRequestInterception", Params().set("patterns", patterns));
    }

    
    CDPResponse continueInterceptedRequest(const std::string& interceptionId,
                                            const std::string& errorReason = "",
                                            const std::string& rawResponse = "",
                                            const std::string& url = "",
                                            const std::string& method = "",
                                            const std::string& postData = "",
                                            const std::map<std::string, std::string>& headers = {},
                                            const JsonValue& authChallengeResponse = JsonValue()) {
        Params params;
        params.set("interceptionId", interceptionId);
        if (!errorReason.empty()) params.set("errorReason", errorReason);
        if (!rawResponse.empty()) params.set("rawResponse", rawResponse);
        if (!url.empty()) params.set("url", url);
        if (!method.empty()) params.set("method", method);
        if (!postData.empty()) params.set("postData", postData);
        if (!headers.empty()) {
            JsonObject headersObj;
            for (const auto& [key, value] : headers) headersObj[key] = value;
            params.set("headers", headersObj);
        }
        if (!authChallengeResponse.isNull()) params.set("authChallengeResponse", authChallengeResponse);
        return call("continueInterceptedRequest", params);
    }

    
    void onRequestWillBeSent(std::function<void(const std::string& requestId,
                                                 const std::string& loaderId,
                                                 const std::string& documentURL,
                                                 const JsonValue& request,
                                                 double timestamp,
                                                 double wallTime,
                                                 const JsonValue& initiator,
                                                 const JsonValue& redirectResponse,
                                                 const std::string& type,
                                                 const std::string& frameId)> callback) {
        on("requestWillBeSent", [callback](const CDPEvent& event) {
            callback(
                event.params["requestId"].getString(),
                event.params["loaderId"].getString(),
                event.params["documentURL"].getString(),
                event.params["request"],
                event.params["timestamp"].getNumber(),
                event.params["wallTime"].getNumber(),
                event.params["initiator"],
                event.params["redirectResponse"],
                event.params["type"].getString(),
                event.params["frameId"].getString()
            );
        });
    }

    void onResponseReceived(std::function<void(const std::string& requestId,
                                                const std::string& loaderId,
                                                double timestamp,
                                                const std::string& type,
                                                const JsonValue& response,
                                                const std::string& frameId)> callback) {
        on("responseReceived", [callback](const CDPEvent& event) {
            callback(
                event.params["requestId"].getString(),
                event.params["loaderId"].getString(),
                event.params["timestamp"].getNumber(),
                event.params["type"].getString(),
                event.params["response"],
                event.params["frameId"].getString()
            );
        });
    }

    void onLoadingFinished(std::function<void(const std::string& requestId,
                                               double timestamp,
                                               double encodedDataLength)> callback) {
        on("loadingFinished", [callback](const CDPEvent& event) {
            callback(
                event.params["requestId"].getString(),
                event.params["timestamp"].getNumber(),
                event.params["encodedDataLength"].getNumber()
            );
        });
    }

    void onLoadingFailed(std::function<void(const std::string& requestId,
                                             double timestamp,
                                             const std::string& type,
                                             const std::string& errorText,
                                             bool canceled,
                                             const std::string& blockedReason)> callback) {
        on("loadingFailed", [callback](const CDPEvent& event) {
            callback(
                event.params["requestId"].getString(),
                event.params["timestamp"].getNumber(),
                event.params["type"].getString(),
                event.params["errorText"].getString(),
                event.params["canceled"].getBool(),
                event.params["blockedReason"].getString()
            );
        });
    }

    void onDataReceived(std::function<void(const std::string& requestId,
                                            double timestamp,
                                            int dataLength,
                                            int encodedDataLength)> callback) {
        on("dataReceived", [callback](const CDPEvent& event) {
            callback(
                event.params["requestId"].getString(),
                event.params["timestamp"].getNumber(),
                event.params["dataLength"].getInt(),
                event.params["encodedDataLength"].getInt()
            );
        });
    }

    void onWebSocketCreated(std::function<void(const std::string& requestId,
                                                const std::string& url,
                                                const JsonValue& initiator)> callback) {
        on("webSocketCreated", [callback](const CDPEvent& event) {
            callback(
                event.params["requestId"].getString(),
                event.params["url"].getString(),
                event.params["initiator"]
            );
        });
    }

    void onWebSocketFrameSent(std::function<void(const std::string& requestId,
                                                  double timestamp,
                                                  const JsonValue& response)> callback) {
        on("webSocketFrameSent", [callback](const CDPEvent& event) {
            callback(
                event.params["requestId"].getString(),
                event.params["timestamp"].getNumber(),
                event.params["response"]
            );
        });
    }

    void onWebSocketFrameReceived(std::function<void(const std::string& requestId,
                                                      double timestamp,
                                                      const JsonValue& response)> callback) {
        on("webSocketFrameReceived", [callback](const CDPEvent& event) {
            callback(
                event.params["requestId"].getString(),
                event.params["timestamp"].getNumber(),
                event.params["response"]
            );
        });
    }

    void onEventSourceMessageReceived(std::function<void(const std::string& requestId,
                                                          double timestamp,
                                                          const std::string& eventName,
                                                          const std::string& eventId,
                                                          const std::string& data)> callback) {
        on("eventSourceMessageReceived", [callback](const CDPEvent& event) {
            callback(
                event.params["requestId"].getString(),
                event.params["timestamp"].getNumber(),
                event.params["eventName"].getString(),
                event.params["eventId"].getString(),
                event.params["data"].getString()
            );
        });
    }
};

} 
