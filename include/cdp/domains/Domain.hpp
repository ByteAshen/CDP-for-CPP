#pragma once

#include "../protocol/CDPConnection.hpp"
#include <string>
#include <functional>
#include <optional>
#include <future>
#include <memory>

namespace cdp {


class Params {
public:
    Params() = default;

    template<typename T>
    Params& set(const std::string& key, const T& value) {
        if constexpr (std::is_same_v<T, bool>) {
            data_[key] = value;
        } else if constexpr (std::is_same_v<T, int> || std::is_same_v<T, int64_t>) {
            data_[key] = static_cast<double>(value);
        } else if constexpr (std::is_same_v<T, double> || std::is_same_v<T, float>) {
            data_[key] = static_cast<double>(value);
        } else if constexpr (std::is_same_v<T, std::string> || std::is_same_v<T, const char*>) {
            data_[key] = std::string(value);
        } else if constexpr (std::is_same_v<T, JsonValue>) {
            data_[key] = value;
        } else if constexpr (std::is_same_v<T, JsonArray>) {
            data_[key] = value;
        } else if constexpr (std::is_same_v<T, JsonObject>) {
            data_[key] = value;
        } else {
            data_[key] = value;
        }
        return *this;
    }

    
    Params& setArray(const std::string& key, const std::vector<std::string>& values) {
        JsonArray arr;
        for (const auto& v : values) arr.push_back(v);
        data_[key] = arr;
        return *this;
    }

    Params& setArray(const std::string& key, const std::vector<int>& values) {
        JsonArray arr;
        for (const auto& v : values) arr.push_back(static_cast<double>(v));
        data_[key] = arr;
        return *this;
    }

    Params& setArray(const std::string& key, const JsonArray& values) {
        data_[key] = values;
        return *this;
    }

    
    template<typename T>
    Params& setOptional(const std::string& key, const std::optional<T>& value) {
        if (value.has_value()) {
            set(key, value.value());
        }
        return *this;
    }

    
    Params& setObject(const std::string& key, const Params& nested) {
        data_[key] = nested.build();
        return *this;
    }

    JsonValue build() const { return data_; }
    operator JsonValue() const { return data_; }

    const JsonObject& data() const { return data_; }

private:
    JsonObject data_;
};


class EventToken {
public:
    EventToken() = default;

    EventToken(CDPConnection* conn, std::string eventName)
        : connection_(conn), eventName_(std::move(eventName)), active_(true) {}

    
    EventToken(const EventToken&) = delete;
    EventToken& operator=(const EventToken&) = delete;

    EventToken(EventToken&& other) noexcept
        : connection_(other.connection_), eventName_(std::move(other.eventName_)), active_(other.active_) {
        other.connection_ = nullptr;
        other.active_ = false;
    }

    EventToken& operator=(EventToken&& other) noexcept {
        if (this != &other) {
            release();
            connection_ = other.connection_;
            eventName_ = std::move(other.eventName_);
            active_ = other.active_;
            other.connection_ = nullptr;
            other.active_ = false;
        }
        return *this;
    }

    ~EventToken() {
        release();
    }

    
    void release() {
        if (active_ && connection_) {
            
            
            connection_->removeEventHandler(eventName_);
            active_ = false;
            connection_ = nullptr;  
        }
    }

    
    bool isActive() const { return active_; }
    explicit operator bool() const { return active_; }

    
    const std::string& eventName() const { return eventName_; }

private:
    CDPConnection* connection_ = nullptr;
    std::string eventName_;
    bool active_ = false;
};


class Domain {
public:
    explicit Domain(CDPConnection& connection, const std::string& name)
        : connection_(connection), domainName_(name) {}

    virtual ~Domain() = default;

    
    CDPResponse enable() {
        auto r = call("enable");
        if (!r.hasError) enabled_ = true;
        return r;
    }

    CDPResponse enable(const Params& params) {
        auto r = call("enable", params);
        if (!r.hasError) enabled_ = true;
        return r;
    }

    
    CDPResponse disable() {
        clearHandlers();  
        auto r = call("disable");
        enabled_ = false;  
        return r;
    }

    
    bool isEnabled() const { return enabled_; }

    
    CDPResponse call(const std::string& method) {
        return connection_.sendCommandSync(domainName_ + "." + method);
    }

    CDPResponse call(const std::string& method, const Params& params) {
        return connection_.sendCommandSync(domainName_ + "." + method, params.build());
    }

    CDPResponse call(const std::string& method, const JsonValue& params) {
        return connection_.sendCommandSync(domainName_ + "." + method, params);
    }

    
    CDPResponse call(const std::string& method, const Params& params, int timeoutMs) {
        return connection_.sendCommandSync(domainName_ + "." + method, params.build(), timeoutMs);
    }

    CDPResponse call(const std::string& method, int timeoutMs) {
        return connection_.sendCommandSync(domainName_ + "." + method, JsonValue(), timeoutMs);
    }

    
    int64_t callAsync(const std::string& method, ResponseCallback callback = nullptr) {
        return connection_.sendCommand(domainName_ + "." + method, callback);
    }

    int64_t callAsync(const std::string& method, const Params& params, ResponseCallback callback = nullptr) {
        return connection_.sendCommand(domainName_ + "." + method, params.build(), callback);
    }

    
    void on(const std::string& event, EventCallback callback) {
        connection_.onEvent(domainName_ + "." + event, callback);
    }

    void off(const std::string& event) {
        connection_.removeEventHandler(domainName_ + "." + event);
    }

    
    [[nodiscard]] EventToken onScoped(const std::string& event, EventCallback callback) {
        std::string fullEvent = domainName_ + "." + event;
        connection_.onEvent(fullEvent, std::move(callback));
        return EventToken(&connection_, fullEvent);
    }

    
    std::future<CDPEvent> once(const std::string& event) {
        auto promise = std::make_shared<std::promise<CDPEvent>>();
        std::string fullEvent = domainName_ + "." + event;

        
        CDPConnection* connPtr = &connection_;
        std::string eventCopy = fullEvent;  

        try {
            connection_.onEvent(fullEvent, [promise, connPtr, eventCopy](const CDPEvent& e) mutable {
                
                try {
                    promise->set_value(e);
                } catch (const std::future_error&) {
                    
                }
                
                connPtr->removeEventHandler(eventCopy);
            });
        } catch (const std::exception& ex) {
            
            promise->set_exception(std::make_exception_ptr(
                std::runtime_error(std::string("Failed to register event handler: ") + ex.what())));
        } catch (...) {
            promise->set_exception(std::make_exception_ptr(
                std::runtime_error("Failed to register event handler: unknown error")));
        }

        return promise->get_future();
    }

    
    std::future<CDPEvent> once(const std::string& event, std::function<bool(const CDPEvent&)> predicate) {
        auto promise = std::make_shared<std::promise<CDPEvent>>();
        std::string fullEvent = domainName_ + "." + event;

        
        CDPConnection* connPtr = &connection_;
        std::string eventCopy = fullEvent;
        auto pred = std::make_shared<std::function<bool(const CDPEvent&)>>(std::move(predicate));

        try {
            connection_.onEvent(fullEvent, [promise, connPtr, eventCopy, pred](const CDPEvent& e) mutable {
                if ((*pred)(e)) {
                    try {
                        promise->set_value(e);
                    } catch (const std::future_error&) {
                        
                    }
                    connPtr->removeEventHandler(eventCopy);
                }
                
            });
        } catch (const std::exception& ex) {
            promise->set_exception(std::make_exception_ptr(
                std::runtime_error(std::string("Failed to register event handler: ") + ex.what())));
        } catch (...) {
            promise->set_exception(std::make_exception_ptr(
                std::runtime_error("Failed to register event handler: unknown error")));
        }

        return promise->get_future();
    }

    
    std::optional<CDPEvent> waitFor(const std::string& event, int timeoutMs) {
        auto future = once(event);
        auto status = future.wait_for(std::chrono::milliseconds(timeoutMs));
        if (status == std::future_status::ready) {
            return future.get();
        }
        
        off(event);
        return std::nullopt;
    }

    
    void clearHandlers() {
        connection_.removeEventHandlersByPrefix(domainName_ + ".");
    }

    
    const std::string& name() const { return domainName_; }

protected:
    CDPConnection& connection_;
    std::string domainName_;
    bool enabled_ = false;
};


#define CDP_METHOD(methodName) \
    CDPResponse methodName() { return call(#methodName); } \
    CDPResponse methodName(const Params& params) { return call(#methodName, params); }

#define CDP_METHOD_ASYNC(methodName) \
    int64_t methodName##Async(ResponseCallback cb = nullptr) { return callAsync(#methodName, cb); } \
    int64_t methodName##Async(const Params& params, ResponseCallback cb = nullptr) { return callAsync(#methodName, params, cb); }

} 
