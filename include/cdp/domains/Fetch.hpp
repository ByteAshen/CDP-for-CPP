#pragma once

#include "Domain.hpp"

namespace cdp {


struct RequestPattern {
    std::string urlPattern = "*";
    std::string resourceType;  
    std::string requestStage = "Request";  

    JsonValue toJson() const {
        JsonObject obj;
        if (!urlPattern.empty()) obj["urlPattern"] = urlPattern;
        if (!resourceType.empty()) obj["resourceType"] = resourceType;
        if (!requestStage.empty()) obj["requestStage"] = requestStage;
        return obj;
    }

    static RequestPattern all() { return {"*", "", "Request"}; }
    static RequestPattern url(const std::string& pattern) { return {pattern, "", "Request"}; }
    static RequestPattern type(const std::string& resType) { return {"*", resType, "Request"}; }
};


struct HeaderEntry {
    std::string name;
    std::string value;

    JsonValue toJson() const {
        JsonObject obj;
        obj["name"] = name;
        obj["value"] = value;
        return obj;
    }

    
    static std::vector<HeaderEntry> fromRequest(const JsonValue& request) {
        std::vector<HeaderEntry> headers;
        if (request.isObject() && request.contains("headers")) {
            const auto& hdrs = request["headers"];
            if (hdrs.isObject()) {
                for (const auto& kv : hdrs.asObject()) {
                    headers.push_back(HeaderEntry{kv.first, kv.second.getString()});
                }
            }
        }
        return headers;
    }

    
    static void set(std::vector<HeaderEntry>& headers, const std::string& name, const std::string& value) {
        for (auto& h : headers) {
            if (h.name == name) {
                h.value = value;
                return;
            }
        }
        headers.push_back({name, value});
    }

    
    static bool remove(std::vector<HeaderEntry>& headers, const std::string& name) {
        for (auto it = headers.begin(); it != headers.end(); ++it) {
            if (it->name == name) {
                headers.erase(it);
                return true;
            }
        }
        return false;
    }
};


struct AuthChallengeResponse {
    std::string response;  
    std::string username;
    std::string password;

    JsonValue toJson() const {
        JsonObject obj;
        obj["response"] = response;
        if (!username.empty()) obj["username"] = username;
        if (!password.empty()) obj["password"] = password;
        return obj;
    }

    static AuthChallengeResponse cancel() { return {"CancelAuth", "", ""}; }
    static AuthChallengeResponse defaultResponse() { return {"Default", "", ""}; }
    static AuthChallengeResponse credentials(const std::string& user, const std::string& pass) {
        return {"ProvideCredentials", user, pass};
    }
};


class Fetch : public Domain {
public:
    explicit Fetch(CDPConnection& connection) : Domain(connection, "Fetch") {}

    
    CDPResponse enable(const std::vector<RequestPattern>& patterns = {},
                       bool handleAuthRequests = false) {
        
        
        if (handleAuthRequests) {
            authHandlingRequired_ = true;
        }

        
        std::vector<RequestPattern> mergedPatterns = patterns;
        if (!patterns.empty()) {
            for (const auto& existing : currentPatterns_) {
                bool found = false;
                for (const auto& p : patterns) {
                    if (p.urlPattern == existing.urlPattern &&
                        p.resourceType == existing.resourceType &&
                        p.requestStage == existing.requestStage) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    mergedPatterns.push_back(existing);
                }
            }
        }
        currentPatterns_ = mergedPatterns;

        Params params;
        if (!mergedPatterns.empty()) {
            JsonArray arr;
            for (const auto& p : mergedPatterns) arr.push_back(p.toJson());
            params.set("patterns", arr);
        }
        if (authHandlingRequired_) {
            params.set("handleAuthRequests", true);
        }
        return call("enable", params);
    }

    
    CDPResponse disable() {
        authHandlingRequired_ = false;
        currentPatterns_.clear();
        return call("disable");
    }

    
    bool isAuthHandlingEnabled() const { return authHandlingRequired_; }

    
    void requireAuthHandling(bool require = true) { authHandlingRequired_ = require; }

    
    CDPResponse continueRequest(const std::string& requestId) {
        return call("continueRequest", Params().set("requestId", requestId));
    }

    
    CDPResponse continueRequest(const std::string& requestId,
                                 const std::string& url,
                                 const std::string& method = "",
                                 const std::string& postData = "",
                                 const std::vector<HeaderEntry>& headers = {},
                                 bool interceptResponse = false) {
        Params params;
        params.set("requestId", requestId);
        if (!url.empty()) params.set("url", url);
        if (!method.empty()) params.set("method", method);
        if (!postData.empty()) params.set("postData", postData);
        if (!headers.empty()) {
            JsonArray arr;
            for (const auto& h : headers) arr.push_back(h.toJson());
            params.set("headers", arr);
        }
        if (interceptResponse) params.set("interceptResponse", true);
        return call("continueRequest", params);
    }

    
    CDPResponse failRequest(const std::string& requestId, const std::string& errorReason) {
        
        return call("failRequest", Params()
            .set("requestId", requestId)
            .set("errorReason", errorReason));
    }

    
    CDPResponse fulfillRequest(const std::string& requestId,
                                int responseCode,
                                const std::vector<HeaderEntry>& responseHeaders = {},
                                const std::string& body = "",
                                const std::string& responsePhrase = "",
                                const std::string& binaryResponseHeaders = "") {
        Params params;
        params.set("requestId", requestId);
        params.set("responseCode", responseCode);

        if (!responseHeaders.empty()) {
            JsonArray arr;
            for (const auto& h : responseHeaders) arr.push_back(h.toJson());
            params.set("responseHeaders", arr);
        }
        if (!binaryResponseHeaders.empty()) params.set("binaryResponseHeaders", binaryResponseHeaders);
        if (!body.empty()) params.set("body", body);  
        if (!responsePhrase.empty()) params.set("responsePhrase", responsePhrase);

        return call("fulfillRequest", params);
    }

    
    CDPResponse continueResponse(const std::string& requestId,
                                  int responseCode = 0,
                                  const std::string& responsePhrase = "",
                                  const std::vector<HeaderEntry>& responseHeaders = {},
                                  const std::string& binaryResponseHeaders = "") {
        Params params;
        params.set("requestId", requestId);
        if (responseCode > 0) params.set("responseCode", responseCode);
        if (!responsePhrase.empty()) params.set("responsePhrase", responsePhrase);
        if (!responseHeaders.empty()) {
            JsonArray arr;
            for (const auto& h : responseHeaders) arr.push_back(h.toJson());
            params.set("responseHeaders", arr);
        }
        if (!binaryResponseHeaders.empty()) params.set("binaryResponseHeaders", binaryResponseHeaders);
        return call("continueResponse", params);
    }

    
    CDPResponse continueWithAuth(const std::string& requestId,
                                  const AuthChallengeResponse& authResponse) {
        return call("continueWithAuth", Params()
            .set("requestId", requestId)
            .set("authChallengeResponse", authResponse.toJson()));
    }

    
    void continueRequestAsync(const std::string& requestId) {
        callAsync("continueRequest", Params().set("requestId", requestId));
    }

    
    void continueRequestAsync(const std::string& requestId,
                               const std::vector<HeaderEntry>& headers,
                               const std::string& url = "",
                               const std::string& method = "",
                               const std::string& postData = "") {
        Params params;
        params.set("requestId", requestId);
        if (!url.empty()) params.set("url", url);
        if (!method.empty()) params.set("method", method);
        if (!postData.empty()) params.set("postData", postData);
        if (!headers.empty()) {
            JsonArray arr;
            for (const auto& h : headers) arr.push_back(h.toJson());
            params.set("headers", arr);
        }
        callAsync("continueRequest", params);
    }

    
    void failRequestAsync(const std::string& requestId, const std::string& errorReason) {
        callAsync("failRequest", Params()
            .set("requestId", requestId)
            .set("errorReason", errorReason));
    }

    
    void fulfillRequestAsync(const std::string& requestId,
                              int responseCode,
                              const std::vector<HeaderEntry>& responseHeaders = {},
                              const std::string& body = "",
                              const std::string& responsePhrase = "",
                              const std::string& binaryResponseHeaders = "") {
        Params params;
        params.set("requestId", requestId);
        params.set("responseCode", responseCode);

        if (!responseHeaders.empty()) {
            JsonArray arr;
            for (const auto& h : responseHeaders) arr.push_back(h.toJson());
            params.set("responseHeaders", arr);
        }
        if (!binaryResponseHeaders.empty()) params.set("binaryResponseHeaders", binaryResponseHeaders);
        if (!body.empty()) params.set("body", body);
        if (!responsePhrase.empty()) params.set("responsePhrase", responsePhrase);

        callAsync("fulfillRequest", params);
    }

    
    void continueResponseAsync(const std::string& requestId,
                                int responseCode = 0,
                                const std::string& responsePhrase = "",
                                const std::vector<HeaderEntry>& responseHeaders = {},
                                const std::string& binaryResponseHeaders = "") {
        Params params;
        params.set("requestId", requestId);
        if (responseCode > 0) params.set("responseCode", responseCode);
        if (!responsePhrase.empty()) params.set("responsePhrase", responsePhrase);
        if (!responseHeaders.empty()) {
            JsonArray arr;
            for (const auto& h : responseHeaders) arr.push_back(h.toJson());
            params.set("responseHeaders", arr);
        }
        if (!binaryResponseHeaders.empty()) params.set("binaryResponseHeaders", binaryResponseHeaders);
        callAsync("continueResponse", params);
    }

    
    void continueWithAuthAsync(const std::string& requestId,
                                const AuthChallengeResponse& authResponse) {
        callAsync("continueWithAuth", Params()
            .set("requestId", requestId)
            .set("authChallengeResponse", authResponse.toJson()));
    }

    
    CDPResponse getResponseBody(const std::string& requestId) {
        return call("getResponseBody", Params().set("requestId", requestId));
    }

    
    CDPResponse takeResponseBodyAsStream(const std::string& requestId) {
        return call("takeResponseBodyAsStream", Params().set("requestId", requestId));
    }

    
    void onRequestPaused(std::function<void(const std::string& requestId,
                                             const JsonValue& request,
                                             const std::string& frameId,
                                             const std::string& resourceType,
                                             const JsonValue& responseErrorReason,
                                             int responseStatusCode,
                                             const std::string& responseStatusText,
                                             const JsonValue& responseHeaders,
                                             const std::string& networkId)> callback) {
        on("requestPaused", [callback](const CDPEvent& event) {
            callback(
                event.params["requestId"].getString(),
                event.params["request"],
                event.params["frameId"].getString(),
                event.params["resourceType"].getString(),
                event.params["responseErrorReason"],
                event.params["responseStatusCode"].getInt(),
                event.params["responseStatusText"].getString(),
                event.params["responseHeaders"],
                event.params["networkId"].getString()
            );
        });
    }

    void onAuthRequired(std::function<void(const std::string& requestId,
                                            const JsonValue& request,
                                            const std::string& frameId,
                                            const std::string& resourceType,
                                            const JsonValue& authChallenge)> callback) {
        on("authRequired", [callback](const CDPEvent& event) {
            callback(
                event.params["requestId"].getString(),
                event.params["request"],
                event.params["frameId"].getString(),
                event.params["resourceType"].getString(),
                event.params["authChallenge"]
            );
        });
    }

private:
    bool authHandlingRequired_ = false;
    std::vector<RequestPattern> currentPatterns_;
};

} 
