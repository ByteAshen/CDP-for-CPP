#pragma once

#include <string>
#include <variant>
#include <optional>
#include <type_traits>
#include <stdexcept>
#include "../protocol/CDPConnection.hpp"

namespace cdp {
namespace highlevel {


enum class ErrorCategory {
    None = 0,           
    Network,            
    Protocol,           
    Timeout,            
    ElementNotFound,    
    ElementStale,       
    ElementNotVisible,  
    ElementNotEnabled,  
    Navigation,         
    JavaScript,         
    InvalidArgument,    
    NotSupported,       
    Internal,           
    Cancelled,          
};


namespace ErrorCode {
    
    constexpr int ConnectionFailed = 100;
    constexpr int ConnectionClosed = 101;
    constexpr int WebSocketError = 102;

    
    constexpr int ProtocolError = 200;
    constexpr int InvalidResponse = 201;
    constexpr int MethodNotFound = 202;

    
    constexpr int Timeout = 300;
    constexpr int WaitTimeout = 301;
    constexpr int NavigationTimeout = 302;
    constexpr int ResponseTimeout = 303;

    
    constexpr int ElementNotFound = 400;
    constexpr int ElementStale = 401;
    constexpr int ElementNotVisible = 402;
    constexpr int ElementNotEnabled = 403;
    constexpr int ElementNotInteractable = 404;
    constexpr int ElementDetached = 405;
    constexpr int NoSuchFrame = 406;

    
    constexpr int NavigationFailed = 500;
    constexpr int PageCrashed = 501;
    constexpr int CertificateError = 502;
    constexpr int PageNotLoaded = 503;

    
    constexpr int JavaScriptError = 600;
    constexpr int JavaScriptException = 601;
    constexpr int EvaluationFailed = 602;

    
    constexpr int InvalidArgument = 700;
    constexpr int InvalidSelector = 701;
    constexpr int InvalidUrl = 702;

    
    constexpr int NotSupported = 800;
    constexpr int Cancelled = 801;
    constexpr int Internal = 900;
}


inline ErrorCategory categoryFromCode(int code) {
    if (code == 0) return ErrorCategory::None;
    int category = code / 100;
    switch (category) {
        case 1: return ErrorCategory::Network;
        case 2: return ErrorCategory::Protocol;
        case 3: return ErrorCategory::Timeout;
        case 4: {
            if (code == ErrorCode::ElementNotFound) return ErrorCategory::ElementNotFound;
            if (code == ErrorCode::ElementStale || code == ErrorCode::ElementDetached)
                return ErrorCategory::ElementStale;
            if (code == ErrorCode::ElementNotVisible) return ErrorCategory::ElementNotVisible;
            if (code == ErrorCode::ElementNotEnabled) return ErrorCategory::ElementNotEnabled;
            return ErrorCategory::ElementNotFound;
        }
        case 5: return ErrorCategory::Navigation;
        case 6: return ErrorCategory::JavaScript;
        case 7: return ErrorCategory::InvalidArgument;
        case 8: {
            if (code == ErrorCode::Cancelled) return ErrorCategory::Cancelled;
            return ErrorCategory::NotSupported;
        }
        case 9: return ErrorCategory::Internal;
        default: return ErrorCategory::Internal;
    }
}


inline ErrorCategory toErrorCategory(CDPErrorCategory cdpCategory) {
    switch (cdpCategory) {
        case CDPErrorCategory::None: return ErrorCategory::None;
        case CDPErrorCategory::Protocol: return ErrorCategory::Protocol;
        case CDPErrorCategory::Target: return ErrorCategory::Navigation;  
        case CDPErrorCategory::Timeout: return ErrorCategory::Timeout;
        case CDPErrorCategory::Connection: return ErrorCategory::Network;
        case CDPErrorCategory::JavaScript: return ErrorCategory::JavaScript;
        case CDPErrorCategory::InvalidState: return ErrorCategory::InvalidArgument;
        case CDPErrorCategory::NotFound: return ErrorCategory::ElementNotFound;
        case CDPErrorCategory::Internal: return ErrorCategory::Internal;
    }
    return ErrorCategory::Internal;
}


inline int toErrorCode(const CDPResponse& response) {
    if (!response.hasError) return 0;

    switch (response.errorCategory()) {
        case CDPErrorCategory::None: return 0;
        case CDPErrorCategory::Protocol: return ErrorCode::ProtocolError;
        case CDPErrorCategory::Target: return ErrorCode::PageCrashed;
        case CDPErrorCategory::Timeout: return ErrorCode::Timeout;
        case CDPErrorCategory::Connection: return ErrorCode::ConnectionClosed;
        case CDPErrorCategory::JavaScript: return ErrorCode::JavaScriptError;
        case CDPErrorCategory::InvalidState: return ErrorCode::InvalidArgument;
        case CDPErrorCategory::NotFound: return ErrorCode::ElementNotFound;
        case CDPErrorCategory::Internal: return ErrorCode::Internal;
    }
    return ErrorCode::Internal;
}


struct ErrorContext {
    std::string selector;        
    std::string url;             
    std::string operation;       
    std::string cdpMethod;       
    std::string cdpRequest;      
    std::string cdpResponse;     
    std::string frameId;         
    std::string targetId;        
    int64_t nodeId = 0;          
    int attemptNumber = 0;       
    int maxAttempts = 0;         

    bool empty() const noexcept {
        return selector.empty() && url.empty() && operation.empty() &&
               cdpMethod.empty() && cdpRequest.empty() && cdpResponse.empty();
    }

    std::string format() const {
        std::string result;
        if (!operation.empty()) result += "operation=" + operation;
        if (!selector.empty()) result += (result.empty() ? "" : ", ") + std::string("selector=") + selector;
        if (!url.empty()) result += (result.empty() ? "" : ", ") + std::string("url=") + url;
        if (!cdpMethod.empty()) result += (result.empty() ? "" : ", ") + std::string("method=") + cdpMethod;
        if (nodeId != 0) result += (result.empty() ? "" : ", ") + std::string("nodeId=") + std::to_string(nodeId);
        if (attemptNumber > 0) result += (result.empty() ? "" : ", ") + std::string("attempt=") + std::to_string(attemptNumber) + "/" + std::to_string(maxAttempts);
        return result;
    }
};


struct Error {
    int code = 0;
    std::string message;
    std::string context;         
    ErrorContext richContext;    

    Error() = default;
    explicit Error(const std::string& msg) : message(msg) {}
    Error(int c, const std::string& msg) : code(c), message(msg) {}
    Error(int c, const std::string& msg, const std::string& ctx)
        : code(c), message(msg), context(ctx) {}
    Error(int c, const std::string& msg, ErrorContext ctx)
        : code(c), message(msg), richContext(std::move(ctx)) {}

    
    ErrorCategory category() const noexcept { return categoryFromCode(code); }

    
    bool isTimeout() const noexcept { return category() == ErrorCategory::Timeout; }
    bool isElementNotFound() const noexcept { return category() == ErrorCategory::ElementNotFound; }
    bool isElementStale() const noexcept { return category() == ErrorCategory::ElementStale; }
    bool isNetwork() const noexcept { return category() == ErrorCategory::Network; }
    bool isCancelled() const noexcept { return category() == ErrorCategory::Cancelled; }
    bool isRetryable() const noexcept {
        return isTimeout() || isNetwork() || isElementStale();
    }

    std::string fullMessage() const {
        std::string result = message;
        if (!context.empty()) {
            result += " (context: " + context + ")";
        }
        if (!richContext.empty()) {
            result += " [" + richContext.format() + "]";
        }
        return result;
    }

    
    Error& withSelector(const std::string& sel) { richContext.selector = sel; return *this; }
    Error& withUrl(const std::string& u) { richContext.url = u; return *this; }
    Error& withOperation(const std::string& op) { richContext.operation = op; return *this; }
    Error& withCdpMethod(const std::string& method) { richContext.cdpMethod = method; return *this; }
    Error& withCdpRequest(const std::string& req) {
        richContext.cdpRequest = req.length() > 1000 ? req.substr(0, 1000) + "..." : req;
        return *this;
    }
    Error& withCdpResponse(const std::string& resp) {
        richContext.cdpResponse = resp.length() > 1000 ? resp.substr(0, 1000) + "..." : resp;
        return *this;
    }
    Error& withNodeId(int64_t id) { richContext.nodeId = id; return *this; }
    Error& withAttempt(int attempt, int maxAttempts) {
        richContext.attemptNumber = attempt;
        richContext.maxAttempts = maxAttempts;
        return *this;
    }

    
    static Error timeout(const std::string& operation, int timeoutMs) {
        return Error(ErrorCode::Timeout,
                     operation + " timed out after " + std::to_string(timeoutMs) + "ms");
    }

    static Error elementNotFound(const std::string& selector) {
        return Error(ErrorCode::ElementNotFound, "Element not found", selector);
    }

    static Error elementStale(const std::string& reason = "") {
        return Error(ErrorCode::ElementStale,
                     reason.empty() ? "Element is stale (detached from DOM)" : reason);
    }

    static Error elementDetached() {
        return Error(ErrorCode::ElementDetached, "Element has been detached from the document");
    }

    static Error elementNotVisible(const std::string& selector = "") {
        return Error(ErrorCode::ElementNotVisible, "Element is not visible", selector);
    }

    static Error elementNotEnabled(const std::string& selector = "") {
        return Error(ErrorCode::ElementNotEnabled, "Element is not enabled", selector);
    }

    static Error navigationTimeout(int timeoutMs) {
        return Error(ErrorCode::NavigationTimeout,
                     "Navigation timed out after " + std::to_string(timeoutMs) + "ms");
    }

    static Error waitTimeout(const std::string& condition, int timeoutMs) {
        return Error(ErrorCode::WaitTimeout,
                     "Waiting for " + condition + " timed out after " + std::to_string(timeoutMs) + "ms");
    }

    static Error cancelled() {
        return Error(ErrorCode::Cancelled, "Operation was cancelled");
    }

    
    static Error fromCDPResponse(const CDPResponse& response) {
        if (!response.hasError) {
            return Error();  
        }
        return Error(toErrorCode(response), response.errorMessage);
    }
};


template<typename T>
class Result;


struct Void {};


template<typename T>
class Result {
public:
    
    Result(const T& value) : data_(value) {}
    Result(T&& value) : data_(std::move(value)) {}

    
    Result(const Error& error) : data_(error) {}
    Result(Error&& error) : data_(std::move(error)) {}

    
    static Result<T> failure(const std::string& message) {
        return Result<T>(Error(message));
    }

    static Result<T> failure(int code, const std::string& message) {
        return Result<T>(Error(code, message));
    }

    static Result<T> failure(const Error& error) {
        return Result<T>(error);
    }

    
    bool ok() const { return std::holds_alternative<T>(data_); }
    bool hasError() const { return std::holds_alternative<Error>(data_); }
    explicit operator bool() const { return ok(); }

    
    T& value() & {
        if (!ok()) throw std::runtime_error("Result::value() called on error: " + error().message);
        return std::get<T>(data_);
    }

    const T& value() const& {
        if (!ok()) throw std::runtime_error("Result::value() called on error: " + error().message);
        return std::get<T>(data_);
    }

    T&& value() && {
        if (!ok()) throw std::runtime_error("Result::value() called on error: " + error().message);
        return std::move(std::get<T>(data_));
    }

    
    const Error& error() const {
        if (ok()) throw std::runtime_error("Result::error() called on success");
        return std::get<Error>(data_);
    }

    
    [[nodiscard]] const T* value_or_null() const noexcept {
        if (!ok()) return nullptr;
        return &std::get<T>(data_);
    }

    [[nodiscard]] T* value_or_null() noexcept {
        if (!ok()) return nullptr;
        return &std::get<T>(data_);
    }

    
    [[nodiscard]] const Error* error_or_null() const noexcept {
        if (ok()) return nullptr;
        return &std::get<Error>(data_);
    }

    
    T valueOr(const T& defaultValue) const {
        if (ok()) return std::get<T>(data_);
        return defaultValue;
    }

    T valueOr(T&& defaultValue) const {
        if (ok()) return std::get<T>(data_);
        return std::move(defaultValue);
    }

    
    const T* operator->() const {
        if (!ok()) {
            throw std::runtime_error("Result::operator-> called on error: " + error().message);
        }
        return &std::get<T>(data_);
    }

    T* operator->() {
        if (!ok()) {
            throw std::runtime_error("Result::operator-> called on error: " + error().message);
        }
        return &std::get<T>(data_);
    }

    
    template<typename F>
    auto map(F&& func) const -> Result<decltype(func(std::declval<T>()))> {
        using U = decltype(func(std::declval<T>()));
        if (ok()) {
            return Result<U>(func(std::get<T>(data_)));
        }
        return Result<U>(error());
    }

    
    template<typename F>
    auto flatMap(F&& func) const -> decltype(func(std::declval<T>())) {
        using ResultU = decltype(func(std::declval<T>()));
        if (ok()) {
            return func(std::get<T>(data_));
        }
        return ResultU(error());
    }

    
    std::optional<T> toOptional() const {
        if (ok()) return std::get<T>(data_);
        return std::nullopt;
    }

private:
    std::variant<T, Error> data_;
};


template<>
class Result<void> {
public:
    
    Result() : error_(std::nullopt) {}
    Result(Void) : error_(std::nullopt) {}

    
    Result(const Error& error) : error_(error) {}
    Result(Error&& error) : error_(std::move(error)) {}

    
    static Result<void> success() { return Result<void>(); }

    static Result<void> failure(const std::string& message) {
        return Result<void>(Error(message));
    }

    static Result<void> failure(int code, const std::string& message) {
        return Result<void>(Error(code, message));
    }

    static Result<void> failure(const Error& err) {
        return Result<void>(err);
    }

    
    bool ok() const { return !error_.has_value(); }
    bool hasError() const { return error_.has_value(); }
    explicit operator bool() const { return ok(); }

    
    const Error& error() const {
        if (ok()) throw std::runtime_error("Result::error() called on success");
        return *error_;
    }

    
    [[nodiscard]] const Error* error_or_null() const noexcept {
        if (ok()) return nullptr;
        return &(*error_);
    }

    
    template<typename F>
    auto map(F&& func) const -> Result<decltype(func())> {
        using U = decltype(func());
        if (ok()) {
            if constexpr (std::is_void_v<U>) {
                func();
                return Result<void>::success();
            } else {
                return Result<U>(func());
            }
        }
        return Result<U>(*error_);
    }

private:
    std::optional<Error> error_;
};


template<typename T>
Result<T> Ok(T&& value) {
    return Result<T>(std::forward<T>(value));
}

inline Result<void> Ok() {
    return Result<void>::success();
}

template<typename T = void>
Result<T> Err(const std::string& message) {
    return Result<T>::failure(message);
}

template<typename T = void>
Result<T> Err(int code, const std::string& message) {
    return Result<T>::failure(code, message);
}

template<typename T = void>
Result<T> Err(const Error& error) {
    return Result<T>::failure(error);
}

} 
} 
