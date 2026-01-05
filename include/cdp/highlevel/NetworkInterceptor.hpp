#pragma once

#include "../protocol/CDPClient.hpp"
#include "Result.hpp"
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <memory>
#include <mutex>
#include <regex>

namespace cdp {
namespace highlevel {


class NetworkInterceptor;


struct MockResponse {
    int statusCode = 200;
    std::string body;
    std::map<std::string, std::string> headers;

    
    static MockResponse json(const std::string& data, int status = 200) {
        MockResponse r;
        r.statusCode = status;
        r.body = data;
        r.headers["Content-Type"] = "application/json";
        return r;
    }

    static MockResponse html(const std::string& content, int status = 200) {
        MockResponse r;
        r.statusCode = status;
        r.body = content;
        r.headers["Content-Type"] = "text/html";
        return r;
    }

    static MockResponse text(const std::string& content, int status = 200) {
        MockResponse r;
        r.statusCode = status;
        r.body = content;
        r.headers["Content-Type"] = "text/plain";
        return r;
    }

    static MockResponse notFound() {
        MockResponse r;
        r.statusCode = 404;
        r.body = "Not Found";
        r.headers["Content-Type"] = "text/plain";
        return r;
    }

    static MockResponse redirect(const std::string& location, int status = 302) {
        MockResponse r;
        r.statusCode = status;
        r.headers["Location"] = location;
        return r;
    }

    
    MockResponse& withHeader(const std::string& name, const std::string& value) {
        headers[name] = value;
        return *this;
    }

    MockResponse& withContentType(const std::string& contentType) {
        headers["Content-Type"] = contentType;
        return *this;
    }
};


struct InterceptedRequest {
    std::string requestId;
    std::string url;
    std::string method;
    std::map<std::string, std::string> headers;
    std::string postData;
    std::string resourceType;  
};


class InterceptAction {
public:
    enum class Type { Continue, Fulfill, Fail, Defer };

    static InterceptAction continueRequest() {
        return InterceptAction(Type::Continue);
    }

    static InterceptAction continueWithHeaders(const std::map<std::string, std::string>& headers) {
        InterceptAction action(Type::Continue);
        action.modifiedHeaders_ = headers;
        return action;
    }

    static InterceptAction fulfill(const MockResponse& response) {
        InterceptAction action(Type::Fulfill);
        action.mockResponse_ = response;
        return action;
    }

    static InterceptAction fail(const std::string& reason = "Failed") {
        InterceptAction action(Type::Fail);
        action.failReason_ = reason;
        return action;
    }

    static InterceptAction defer() {
        return InterceptAction(Type::Defer);
    }

    Type type() const { return type_; }
    const MockResponse& mockResponse() const { return mockResponse_; }
    const std::map<std::string, std::string>& modifiedHeaders() const { return modifiedHeaders_; }
    const std::string& failReason() const { return failReason_; }

private:
    explicit InterceptAction(Type type) : type_(type) {}

    Type type_;
    MockResponse mockResponse_;
    std::map<std::string, std::string> modifiedHeaders_;
    std::string failReason_;
};


using InterceptCallback = std::function<InterceptAction(const InterceptedRequest&)>;
using ObserveCallback = std::function<void(const InterceptedRequest&)>;


class InterceptorHandle {
public:
    InterceptorHandle() : interceptor_(nullptr), id_(0), active_(false) {}
    InterceptorHandle(NetworkInterceptor* interceptor, uint64_t id);
    ~InterceptorHandle();

    
    InterceptorHandle(InterceptorHandle&& other) noexcept;
    InterceptorHandle& operator=(InterceptorHandle&& other) noexcept;
    InterceptorHandle(const InterceptorHandle&) = delete;
    InterceptorHandle& operator=(const InterceptorHandle&) = delete;

    void remove();
    bool isActive() const { return active_; }
    uint64_t id() const { return id_; }

private:
    NetworkInterceptor* interceptor_;
    uint64_t id_;
    bool active_;
};


class NetworkInterceptor {
public:
    explicit NetworkInterceptor(CDPClient& client);
    ~NetworkInterceptor();

    
    Result<void> enable();
    Result<void> disable();
    bool isEnabled() const { return enabled_; }

    
    InterceptorHandle mockRequest(const std::string& urlPattern, const MockResponse& response);
    InterceptorHandle blockResource(const std::string& urlPattern);
    InterceptorHandle modifyRequestHeaders(const std::string& urlPattern,
                                           const std::map<std::string, std::string>& headers);

    
    InterceptorHandle blockResourceType(const std::string& resourceType);

    
    InterceptorHandle intercept(const std::string& urlPattern, InterceptCallback callback);
    InterceptorHandle observe(const std::string& urlPattern, ObserveCallback callback);

    
    void clear();

    
    void removeRule(uint64_t id);

private:
    struct InterceptRule {
        uint64_t id;
        std::string pattern;
        std::regex regex;
        InterceptCallback callback;
        bool isObserver = false;  
    };

    CDPClient& client_;
    bool enabled_ = false;
    std::vector<InterceptRule> rules_;
    std::mutex rulesMutex_;
    uint64_t nextRuleId_ = 1;
    EventToken requestPausedToken_;

    
    static std::regex patternToRegex(const std::string& pattern);

    
    static bool urlMatches(const std::string& url, const std::regex& regex);

    
    void handleRequestPaused(const JsonValue& params);

    
    void fulfillRequest(const std::string& requestId, const MockResponse& response);

    
    void continueRequest(const std::string& requestId,
                        const std::map<std::string, std::string>* modifiedHeaders = nullptr);

    
    void failRequest(const std::string& requestId, const std::string& reason);
};

} 
} 
