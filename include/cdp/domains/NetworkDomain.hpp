#pragma once

#include "BaseDomain.hpp"
#include <vector>
#include <map>

namespace cdp {


struct RequestInfo {
    std::string requestId;
    std::string loaderId;
    std::string documentURL;
    std::string url;
    std::string method;
    std::map<std::string, std::string> headers;
    std::string postData;
    bool hasPostData = false;
    std::string type;
    double timestamp = 0;

    static RequestInfo fromJson(const JsonValue& json);
};

struct ResponseInfo {
    std::string url;
    int status = 0;
    std::string statusText;
    std::map<std::string, std::string> headers;
    std::string mimeType;
    int encodedDataLength = 0;
    std::string protocol;
    std::string securityState;
    bool fromCache = false;
    bool fromServiceWorker = false;

    static ResponseInfo fromJson(const JsonValue& json);
};


struct Cookie {
    std::string name;
    std::string value;
    std::string domain;
    std::string path;
    double expires = -1;
    int size = 0;
    bool httpOnly = false;
    bool secure = false;
    bool session = false;
    std::string sameSite;
    std::string priority;

    static Cookie fromJson(const JsonValue& json);
    JsonValue toJson() const;
};


struct ResourceTiming {
    double requestTime = 0;
    double proxyStart = -1;
    double proxyEnd = -1;
    double dnsStart = -1;
    double dnsEnd = -1;
    double connectStart = -1;
    double connectEnd = -1;
    double sslStart = -1;
    double sslEnd = -1;
    double sendStart = 0;
    double sendEnd = 0;
    double receiveHeadersEnd = 0;

    static ResourceTiming fromJson(const JsonValue& json);
};


class NetworkDomain : public BaseDomain {
public:
    using BaseDomain::BaseDomain;
    std::string domainName() const override { return "Network"; }

    
    CDPResponse clearBrowserCache();
    CDPResponse clearBrowserCookies();
    CDPResponse setCacheDisabled(bool disabled);

    
    std::vector<Cookie> getCookies(const std::vector<std::string>& urls = {});
    std::vector<Cookie> getAllCookies();
    CDPResponse setCookie(const Cookie& cookie);
    CDPResponse setCookies(const std::vector<Cookie>& cookies);
    CDPResponse deleteCookies(const std::string& name, const std::string& url = "",
                               const std::string& domain = "", const std::string& path = "");

    
    CDPResponse setRequestInterception(const std::vector<std::string>& patterns,
                                        bool enabled = true);
    CDPResponse continueInterceptedRequest(const std::string& interceptionId,
                                            const std::string& errorReason = "",
                                            const std::string& rawResponse = "",
                                            const std::string& url = "",
                                            const std::string& method = "",
                                            const std::string& postData = "",
                                            const std::map<std::string, std::string>& headers = {});

    
    std::pair<std::string, bool> getResponseBody(const std::string& requestId);

    
    CDPResponse emulateNetworkConditions(bool offline, double latency,
                                          double downloadThroughput, double uploadThroughput,
                                          const std::string& connectionType = "");

    
    CDPResponse setUserAgentOverride(const std::string& userAgent,
                                      const std::string& acceptLanguage = "",
                                      const std::string& platform = "");

    
    CDPResponse setExtraHTTPHeaders(const std::map<std::string, std::string>& headers);

    
    CDPResponse setBlockedURLs(const std::vector<std::string>& urls);

    
    CDPResponse enableReportingApi(bool enable = true);

    
    void onRequestWillBeSent(std::function<void(const RequestInfo& request,
                                                 const std::string& frameId,
                                                 const std::string& type)> callback);
    void onResponseReceived(std::function<void(const std::string& requestId,
                                                const std::string& frameId,
                                                const ResponseInfo& response,
                                                const std::string& type)> callback);
    void onLoadingFinished(std::function<void(const std::string& requestId,
                                               double timestamp,
                                               int encodedDataLength)> callback);
    void onLoadingFailed(std::function<void(const std::string& requestId,
                                             double timestamp,
                                             const std::string& type,
                                             const std::string& errorText,
                                             bool canceled)> callback);
    void onDataReceived(std::function<void(const std::string& requestId,
                                            double timestamp,
                                            int dataLength,
                                            int encodedDataLength)> callback);
    void onRequestIntercepted(std::function<void(const std::string& interceptionId,
                                                  const RequestInfo& request,
                                                  const std::string& frameId,
                                                  const std::string& resourceType,
                                                  bool isNavigationRequest)> callback);
    void onWebSocketCreated(std::function<void(const std::string& requestId,
                                                const std::string& url)> callback);
    void onWebSocketFrameSent(std::function<void(const std::string& requestId,
                                                  double timestamp,
                                                  const std::string& payloadData)> callback);
    void onWebSocketFrameReceived(std::function<void(const std::string& requestId,
                                                      double timestamp,
                                                      const std::string& payloadData)> callback);
};

} 
