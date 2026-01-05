

#include "cdp/highlevel/NetworkInterceptor.hpp"
#include "cdp/core/Base64.hpp"
#include <sstream>

namespace cdp {
namespace highlevel {


InterceptorHandle::InterceptorHandle(NetworkInterceptor* interceptor, uint64_t id)
    : interceptor_(interceptor), id_(id), active_(true) {}

InterceptorHandle::~InterceptorHandle() {
    if (active_ && interceptor_) {
        interceptor_->removeRule(id_);
    }
}

InterceptorHandle::InterceptorHandle(InterceptorHandle&& other) noexcept
    : interceptor_(other.interceptor_), id_(other.id_), active_(other.active_) {
    other.active_ = false;
    other.interceptor_ = nullptr;
}

InterceptorHandle& InterceptorHandle::operator=(InterceptorHandle&& other) noexcept {
    if (this != &other) {
        if (active_ && interceptor_) {
            interceptor_->removeRule(id_);
        }
        interceptor_ = other.interceptor_;
        id_ = other.id_;
        active_ = other.active_;
        other.active_ = false;
        other.interceptor_ = nullptr;
    }
    return *this;
}

void InterceptorHandle::remove() {
    if (active_ && interceptor_) {
        interceptor_->removeRule(id_);
        active_ = false;
    }
}


NetworkInterceptor::NetworkInterceptor(CDPClient& client)
    : client_(client) {}

NetworkInterceptor::~NetworkInterceptor() {
    if (enabled_) {
        disable();
    }
}

Result<void> NetworkInterceptor::enable() {
    if (enabled_) {
        return Result<void>::success();
    }

    
    auto resp = client_.Fetch.enable({}, true);  
    if (resp.hasError) {
        return Result<void>::failure(resp.errorCode, resp.errorMessage);
    }

    
    requestPausedToken_ = client_.Fetch.onScoped("requestPaused", [this](const CDPEvent& event) {
        handleRequestPaused(event.params);
    });

    enabled_ = true;
    return Result<void>::success();
}

Result<void> NetworkInterceptor::disable() {
    if (!enabled_) {
        return Result<void>::success();
    }

    
    requestPausedToken_.release();

    
    auto resp = client_.Fetch.disable();
    if (resp.hasError) {
        return Result<void>::failure(resp.errorCode, resp.errorMessage);
    }

    enabled_ = false;
    return Result<void>::success();
}

InterceptorHandle NetworkInterceptor::mockRequest(const std::string& urlPattern, const MockResponse& response) {
    return intercept(urlPattern, [response](const InterceptedRequest&) {
        return InterceptAction::fulfill(response);
    });
}

InterceptorHandle NetworkInterceptor::blockResource(const std::string& urlPattern) {
    return intercept(urlPattern, [](const InterceptedRequest&) {
        return InterceptAction::fail("Blocked");
    });
}

InterceptorHandle NetworkInterceptor::modifyRequestHeaders(const std::string& urlPattern,
                                                           const std::map<std::string, std::string>& headers) {
    return intercept(urlPattern, [headers](const InterceptedRequest&) {
        return InterceptAction::continueWithHeaders(headers);
    });
}

InterceptorHandle NetworkInterceptor::blockResourceType(const std::string& resourceType) {
    return intercept("*", [resourceType](const InterceptedRequest& req) {
        if (req.resourceType == resourceType) {
            return InterceptAction::fail("Blocked by resource type");
        }
        return InterceptAction::defer();
    });
}

InterceptorHandle NetworkInterceptor::intercept(const std::string& urlPattern, InterceptCallback callback) {
    std::lock_guard<std::mutex> lock(rulesMutex_);

    InterceptRule rule;
    rule.id = nextRuleId_++;
    rule.pattern = urlPattern;
    rule.regex = patternToRegex(urlPattern);
    rule.callback = std::move(callback);
    rule.isObserver = false;

    rules_.push_back(std::move(rule));

    return InterceptorHandle(this, rule.id);
}

InterceptorHandle NetworkInterceptor::observe(const std::string& urlPattern, ObserveCallback callback) {
    std::lock_guard<std::mutex> lock(rulesMutex_);

    InterceptRule rule;
    rule.id = nextRuleId_++;
    rule.pattern = urlPattern;
    rule.regex = patternToRegex(urlPattern);
    rule.callback = [callback](const InterceptedRequest& req) {
        callback(req);
        return InterceptAction::defer();
    };
    rule.isObserver = true;

    rules_.push_back(std::move(rule));

    return InterceptorHandle(this, rule.id);
}

void NetworkInterceptor::clear() {
    std::lock_guard<std::mutex> lock(rulesMutex_);
    rules_.clear();
}

void NetworkInterceptor::removeRule(uint64_t id) {
    std::lock_guard<std::mutex> lock(rulesMutex_);
    rules_.erase(
        std::remove_if(rules_.begin(), rules_.end(),
            [id](const InterceptRule& rule) { return rule.id == id; }),
        rules_.end());
}

std::regex NetworkInterceptor::patternToRegex(const std::string& pattern) {
    
    
    std::string regexStr;
    regexStr.reserve(pattern.size() * 2);

    for (char c : pattern) {
        switch (c) {
            case '*':
                regexStr += ".*";
                break;
            case '?':
                regexStr += ".";
                break;
            case '.':
            case '+':
            case '^':
            case '$':
            case '(':
            case ')':
            case '[':
            case ']':
            case '{':
            case '}':
            case '|':
            case '\\':
                regexStr += '\\';
                regexStr += c;
                break;
            default:
                regexStr += c;
                break;
        }
    }

    return std::regex(regexStr, std::regex::icase);
}

bool NetworkInterceptor::urlMatches(const std::string& url, const std::regex& regex) {
    return std::regex_search(url, regex);
}

void NetworkInterceptor::handleRequestPaused(const JsonValue& params) {
    std::string requestId = params["requestId"].getString();
    std::string url = params["request"]["url"].getString();
    std::string method = params["request"]["method"].getString();
    std::string resourceType = params["resourceType"].getString();

    
    InterceptedRequest req;
    req.requestId = requestId;
    req.url = url;
    req.method = method;
    req.resourceType = resourceType;

    
    const auto& headersObj = params["request"]["headers"];
    if (headersObj.isObject()) {
        for (const auto& [key, value] : headersObj.asObject()) {
            req.headers[key] = value.getString();
        }
    }

    
    if (params["request"].contains("postData")) {
        req.postData = params["request"]["postData"].getString();
    }

    
    std::vector<InterceptCallback> matchingCallbacks;
    {
        std::lock_guard<std::mutex> lock(rulesMutex_);
        for (const auto& rule : rules_) {
            if (urlMatches(url, rule.regex)) {
                matchingCallbacks.push_back(rule.callback);
            }
        }
    }

    
    InterceptAction finalAction = InterceptAction::continueRequest();

    for (const auto& callback : matchingCallbacks) {
        try {
            InterceptAction action = callback(req);
            if (action.type() != InterceptAction::Type::Defer) {
                finalAction = action;
                break;
            }
        } catch (const std::exception&) {
            
        }
    }

    
    switch (finalAction.type()) {
        case InterceptAction::Type::Continue:
            if (!finalAction.modifiedHeaders().empty()) {
                continueRequest(requestId, &finalAction.modifiedHeaders());
            } else {
                continueRequest(requestId);
            }
            break;

        case InterceptAction::Type::Fulfill:
            fulfillRequest(requestId, finalAction.mockResponse());
            break;

        case InterceptAction::Type::Fail:
            failRequest(requestId, finalAction.failReason());
            break;

        case InterceptAction::Type::Defer:
            
            continueRequest(requestId);
            break;
    }
}

void NetworkInterceptor::fulfillRequest(const std::string& requestId, const MockResponse& response) {
    
    std::vector<HeaderEntry> headers;
    for (const auto& [name, value] : response.headers) {
        headers.push_back({name, value});
    }

    
    std::string bodyBase64 = Base64::encode(
        std::vector<uint8_t>(response.body.begin(), response.body.end()));

    client_.Fetch.fulfillRequest(requestId, response.statusCode, headers, bodyBase64);
}

void NetworkInterceptor::continueRequest(const std::string& requestId,
                                         const std::map<std::string, std::string>* modifiedHeaders) {
    if (modifiedHeaders && !modifiedHeaders->empty()) {
        std::vector<HeaderEntry> headers;
        for (const auto& [name, value] : *modifiedHeaders) {
            headers.push_back({name, value});
        }
        client_.Fetch.continueRequest(requestId, "", "", "", headers);
    } else {
        client_.Fetch.continueRequest(requestId);
    }
}

void NetworkInterceptor::failRequest(const std::string& requestId, const std::string& reason) {
    
    std::string errorReason = "Failed";
    if (reason == "Blocked") {
        errorReason = "BlockedByClient";
    } else if (reason == "AccessDenied") {
        errorReason = "AccessDenied";
    } else if (reason == "ConnectionRefused") {
        errorReason = "ConnectionRefused";
    }

    client_.Fetch.failRequest(requestId, errorReason);
}

} 
} 
