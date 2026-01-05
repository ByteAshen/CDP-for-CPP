

#include "cdp/domains/NetworkDomain.hpp"

namespace cdp {

RequestInfo RequestInfo::fromJson(const JsonValue& json) {
    RequestInfo req;
    req.requestId = json["requestId"].getString();
    req.loaderId = json["loaderId"].getString();
    req.documentURL = json["documentURL"].getString();

    const auto& request = json.contains("request") ? json["request"] : json;
    req.url = request["url"].getString();
    req.method = request["method"].getString();
    req.postData = request["postData"].getString();
    req.hasPostData = request["hasPostData"].getBool();

    if (request["headers"].isObject()) {
        for (const auto& [key, value] : request["headers"].asObject()) {
            req.headers[key] = value.getString();
        }
    }

    req.type = json["type"].getString();
    req.timestamp = json["timestamp"].getNumber();

    return req;
}

ResponseInfo ResponseInfo::fromJson(const JsonValue& json) {
    ResponseInfo resp;
    resp.url = json["url"].getString();
    resp.status = json["status"].getInt();
    resp.statusText = json["statusText"].getString();
    resp.mimeType = json["mimeType"].getString();
    resp.encodedDataLength = json["encodedDataLength"].getInt();
    resp.protocol = json["protocol"].getString();
    resp.securityState = json["securityState"].getString();
    resp.fromCache = json["fromDiskCache"].getBool() || json["fromPrefetchCache"].getBool();
    resp.fromServiceWorker = json["fromServiceWorker"].getBool();

    if (json["headers"].isObject()) {
        for (const auto& [key, value] : json["headers"].asObject()) {
            resp.headers[key] = value.getString();
        }
    }

    return resp;
}

Cookie Cookie::fromJson(const JsonValue& json) {
    Cookie cookie;
    cookie.name = json["name"].getString();
    cookie.value = json["value"].getString();
    cookie.domain = json["domain"].getString();
    cookie.path = json["path"].getString();
    cookie.expires = json["expires"].getNumber(-1);
    cookie.size = json["size"].getInt();
    cookie.httpOnly = json["httpOnly"].getBool();
    cookie.secure = json["secure"].getBool();
    cookie.session = json["session"].getBool();
    cookie.sameSite = json["sameSite"].getString();
    cookie.priority = json["priority"].getString();
    return cookie;
}

JsonValue Cookie::toJson() const {
    JsonObject obj;
    obj["name"] = name;
    obj["value"] = value;
    obj["domain"] = domain;
    obj["path"] = path;
    if (expires >= 0) obj["expires"] = expires;
    obj["httpOnly"] = httpOnly;
    obj["secure"] = secure;
    if (!sameSite.empty()) obj["sameSite"] = sameSite;
    if (!priority.empty()) obj["priority"] = priority;
    return obj;
}

ResourceTiming ResourceTiming::fromJson(const JsonValue& json) {
    ResourceTiming timing;
    timing.requestTime = json["requestTime"].getNumber();
    timing.proxyStart = json["proxyStart"].getNumber(-1);
    timing.proxyEnd = json["proxyEnd"].getNumber(-1);
    timing.dnsStart = json["dnsStart"].getNumber(-1);
    timing.dnsEnd = json["dnsEnd"].getNumber(-1);
    timing.connectStart = json["connectStart"].getNumber(-1);
    timing.connectEnd = json["connectEnd"].getNumber(-1);
    timing.sslStart = json["sslStart"].getNumber(-1);
    timing.sslEnd = json["sslEnd"].getNumber(-1);
    timing.sendStart = json["sendStart"].getNumber();
    timing.sendEnd = json["sendEnd"].getNumber();
    timing.receiveHeadersEnd = json["receiveHeadersEnd"].getNumber();
    return timing;
}

CDPResponse NetworkDomain::clearBrowserCache() {
    return sendCommand("Network.clearBrowserCache");
}

CDPResponse NetworkDomain::clearBrowserCookies() {
    return sendCommand("Network.clearBrowserCookies");
}

CDPResponse NetworkDomain::setCacheDisabled(bool disabled) {
    JsonObject params;
    params["cacheDisabled"] = disabled;
    return sendCommand("Network.setCacheDisabled", params);
}

std::vector<Cookie> NetworkDomain::getCookies(const std::vector<std::string>& urls) {
    JsonObject params;
    if (!urls.empty()) {
        JsonArray urlsArray;
        for (const auto& url : urls) {
            urlsArray.push_back(url);
        }
        params["urls"] = urlsArray;
    }

    auto response = sendCommand("Network.getCookies", params);
    std::vector<Cookie> cookies;

    if (response.isSuccess() && response.result["cookies"].isArray()) {
        for (size_t i = 0; i < response.result["cookies"].size(); ++i) {
            cookies.push_back(Cookie::fromJson(response.result["cookies"][i]));
        }
    }

    return cookies;
}

std::vector<Cookie> NetworkDomain::getAllCookies() {
    auto response = sendCommand("Network.getAllCookies");
    std::vector<Cookie> cookies;

    if (response.isSuccess() && response.result["cookies"].isArray()) {
        for (size_t i = 0; i < response.result["cookies"].size(); ++i) {
            cookies.push_back(Cookie::fromJson(response.result["cookies"][i]));
        }
    }

    return cookies;
}

CDPResponse NetworkDomain::setCookie(const Cookie& cookie) {
    return sendCommand("Network.setCookie", cookie.toJson());
}

CDPResponse NetworkDomain::setCookies(const std::vector<Cookie>& cookies) {
    JsonObject params;
    JsonArray cookiesArray;
    for (const auto& cookie : cookies) {
        cookiesArray.push_back(cookie.toJson());
    }
    params["cookies"] = cookiesArray;
    return sendCommand("Network.setCookies", params);
}

CDPResponse NetworkDomain::deleteCookies(const std::string& name, const std::string& url,
                                          const std::string& domain, const std::string& path) {
    JsonObject params;
    params["name"] = name;
    if (!url.empty()) params["url"] = url;
    if (!domain.empty()) params["domain"] = domain;
    if (!path.empty()) params["path"] = path;
    return sendCommand("Network.deleteCookies", params);
}

CDPResponse NetworkDomain::setRequestInterception(const std::vector<std::string>& patterns,
                                                   bool enabled) {
    JsonObject params;
    JsonArray patternsArray;
    for (const auto& pattern : patterns) {
        JsonObject patternObj;
        patternObj["urlPattern"] = pattern;
        patternsArray.push_back(patternObj);
    }
    params["patterns"] = patternsArray;
    return sendCommand("Fetch.enable", params);
}

CDPResponse NetworkDomain::continueInterceptedRequest(const std::string& interceptionId,
                                                       const std::string& errorReason,
                                                       const std::string& rawResponse,
                                                       const std::string& url,
                                                       const std::string& method,
                                                       const std::string& postData,
                                                       const std::map<std::string, std::string>& headers) {
    JsonObject params;
    params["requestId"] = interceptionId;

    if (!errorReason.empty()) {
        params["errorReason"] = errorReason;
    }
    if (!rawResponse.empty()) {
        params["rawResponse"] = rawResponse;
    }
    if (!url.empty()) {
        params["url"] = url;
    }
    if (!method.empty()) {
        params["method"] = method;
    }
    if (!postData.empty()) {
        params["postData"] = postData;
    }
    if (!headers.empty()) {
        JsonArray headersArray;
        for (const auto& [name, value] : headers) {
            JsonObject header;
            header["name"] = name;
            header["value"] = value;
            headersArray.push_back(header);
        }
        params["headers"] = headersArray;
    }

    return sendCommand("Fetch.continueRequest", params);
}

std::pair<std::string, bool> NetworkDomain::getResponseBody(const std::string& requestId) {
    JsonObject params;
    params["requestId"] = requestId;

    auto response = sendCommand("Network.getResponseBody", params);
    if (response.isSuccess()) {
        return {response.result["body"].getString(), response.result["base64Encoded"].getBool()};
    }
    return {"", false};
}

CDPResponse NetworkDomain::emulateNetworkConditions(bool offline, double latency,
                                                     double downloadThroughput, double uploadThroughput,
                                                     const std::string& connectionType) {
    JsonObject params;
    params["offline"] = offline;
    params["latency"] = latency;
    params["downloadThroughput"] = downloadThroughput;
    params["uploadThroughput"] = uploadThroughput;
    if (!connectionType.empty()) {
        params["connectionType"] = connectionType;
    }
    return sendCommand("Network.emulateNetworkConditions", params);
}

CDPResponse NetworkDomain::setUserAgentOverride(const std::string& userAgent,
                                                 const std::string& acceptLanguage,
                                                 const std::string& platform) {
    JsonObject params;
    params["userAgent"] = userAgent;
    if (!acceptLanguage.empty()) params["acceptLanguage"] = acceptLanguage;
    if (!platform.empty()) params["platform"] = platform;
    return sendCommand("Network.setUserAgentOverride", params);
}

CDPResponse NetworkDomain::setExtraHTTPHeaders(const std::map<std::string, std::string>& headers) {
    JsonObject params;
    JsonObject headersObj;
    for (const auto& [key, value] : headers) {
        headersObj[key] = value;
    }
    params["headers"] = headersObj;
    return sendCommand("Network.setExtraHTTPHeaders", params);
}

CDPResponse NetworkDomain::setBlockedURLs(const std::vector<std::string>& urls) {
    JsonObject params;
    JsonArray urlsArray;
    for (const auto& url : urls) {
        urlsArray.push_back(url);
    }
    params["urls"] = urlsArray;
    return sendCommand("Network.setBlockedURLs", params);
}

CDPResponse NetworkDomain::enableReportingApi(bool enable) {
    JsonObject params;
    params["enable"] = enable;
    return sendCommand("Network.enableReportingApi", params);
}

void NetworkDomain::onRequestWillBeSent(
    std::function<void(const RequestInfo&, const std::string&, const std::string&)> callback) {
    subscribeEvent("requestWillBeSent", [callback](const CDPEvent& event) {
        callback(
            RequestInfo::fromJson(event.params),
            event.params["frameId"].getString(),
            event.params["type"].getString()
        );
    });
}

void NetworkDomain::onResponseReceived(
    std::function<void(const std::string&, const std::string&, const ResponseInfo&, const std::string&)> callback) {
    subscribeEvent("responseReceived", [callback](const CDPEvent& event) {
        callback(
            event.params["requestId"].getString(),
            event.params["frameId"].getString(),
            ResponseInfo::fromJson(event.params["response"]),
            event.params["type"].getString()
        );
    });
}

void NetworkDomain::onLoadingFinished(
    std::function<void(const std::string&, double, int)> callback) {
    subscribeEvent("loadingFinished", [callback](const CDPEvent& event) {
        callback(
            event.params["requestId"].getString(),
            event.params["timestamp"].getNumber(),
            event.params["encodedDataLength"].getInt()
        );
    });
}

void NetworkDomain::onLoadingFailed(
    std::function<void(const std::string&, double, const std::string&, const std::string&, bool)> callback) {
    subscribeEvent("loadingFailed", [callback](const CDPEvent& event) {
        callback(
            event.params["requestId"].getString(),
            event.params["timestamp"].getNumber(),
            event.params["type"].getString(),
            event.params["errorText"].getString(),
            event.params["canceled"].getBool()
        );
    });
}

void NetworkDomain::onDataReceived(
    std::function<void(const std::string&, double, int, int)> callback) {
    subscribeEvent("dataReceived", [callback](const CDPEvent& event) {
        callback(
            event.params["requestId"].getString(),
            event.params["timestamp"].getNumber(),
            event.params["dataLength"].getInt(),
            event.params["encodedDataLength"].getInt()
        );
    });
}

void NetworkDomain::onRequestIntercepted(
    std::function<void(const std::string&, const RequestInfo&, const std::string&, const std::string&, bool)> callback) {
    connection_.onEvent("Fetch.requestPaused", [callback](const CDPEvent& event) {
        RequestInfo request;
        request.requestId = event.params["requestId"].getString();
        request.url = event.params["request"]["url"].getString();
        request.method = event.params["request"]["method"].getString();
        if (event.params["request"]["headers"].isObject()) {
            for (const auto& [key, value] : event.params["request"]["headers"].asObject()) {
                request.headers[key] = value.getString();
            }
        }
        callback(
            event.params["requestId"].getString(),
            request,
            event.params["frameId"].getString(),
            event.params["resourceType"].getString(),
            event.params["isNavigationRequest"].getBool()
        );
    });
}

void NetworkDomain::onWebSocketCreated(
    std::function<void(const std::string&, const std::string&)> callback) {
    subscribeEvent("webSocketCreated", [callback](const CDPEvent& event) {
        callback(
            event.params["requestId"].getString(),
            event.params["url"].getString()
        );
    });
}

void NetworkDomain::onWebSocketFrameSent(
    std::function<void(const std::string&, double, const std::string&)> callback) {
    subscribeEvent("webSocketFrameSent", [callback](const CDPEvent& event) {
        callback(
            event.params["requestId"].getString(),
            event.params["timestamp"].getNumber(),
            event.params["response"]["payloadData"].getString()
        );
    });
}

void NetworkDomain::onWebSocketFrameReceived(
    std::function<void(const std::string&, double, const std::string&)> callback) {
    subscribeEvent("webSocketFrameReceived", [callback](const CDPEvent& event) {
        callback(
            event.params["requestId"].getString(),
            event.params["timestamp"].getNumber(),
            event.params["response"]["payloadData"].getString()
        );
    });
}

} 
