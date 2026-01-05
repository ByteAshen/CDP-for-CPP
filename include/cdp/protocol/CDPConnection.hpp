

#pragma once

#include "../net/WebSocket.hpp"
#include "../net/HttpClient.hpp"
#include "../core/Json.hpp"
#include <string>
#include <map>
#include <functional>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <future>
#include <chrono>
#include <thread>
#include <queue>
#include <shared_mutex>

namespace cdp {


struct CDPTarget {
    std::string id;
    std::string type;
    std::string title;
    std::string url;
    std::string webSocketDebuggerUrl;
    std::string devtoolsFrontendUrl;
    std::string faviconUrl;
    std::string description;

    static CDPTarget fromJson(const JsonValue& json);
};


struct CDPBrowserInfo {
    std::string browser;
    std::string protocolVersion;
    std::string userAgent;
    std::string v8Version;
    std::string webKitVersion;

    static CDPBrowserInfo fromJson(const JsonValue& json);
};


struct CDPRequest {
    int64_t id;
    std::string method;
    JsonValue params;

    std::string serialize() const;
};


enum class CDPErrorCategory {
    None = 0,           
    Protocol,           
    Target,             
    Timeout,            
    Connection,         
    JavaScript,         
    InvalidState,       
    NotFound,           
    Internal            
};

struct CDPResponse {
    int64_t id = -1;
    JsonValue result;
    bool hasError = false;
    int errorCode = 0;
    std::string errorMessage;

    bool isSuccess() const { return !hasError; }
    explicit operator bool() const { return !hasError; }

    
    CDPErrorCategory errorCategory() const {
        if (!hasError) return CDPErrorCategory::None;

        
        if (errorCode == -32601) return CDPErrorCategory::Protocol;  
        if (errorCode == -32602) return CDPErrorCategory::Protocol;  
        if (errorCode == -32600) return CDPErrorCategory::Protocol;  
        if (errorCode == -32603) return CDPErrorCategory::Internal;  

        
        if (errorCode >= -32099 && errorCode <= -32000) {
            
            if (errorMessage.find("not found") != std::string::npos) return CDPErrorCategory::NotFound;
            if (errorMessage.find("not enabled") != std::string::npos) return CDPErrorCategory::InvalidState;
            if (errorMessage.find("Target closed") != std::string::npos) return CDPErrorCategory::Target;
            if (errorMessage.find("No target") != std::string::npos) return CDPErrorCategory::Target;
            if (errorMessage.find("timeout") != std::string::npos) return CDPErrorCategory::Timeout;
            return CDPErrorCategory::Protocol;
        }

        return CDPErrorCategory::Internal;
    }

    
    bool isTimeout() const { return errorCategory() == CDPErrorCategory::Timeout; }
    bool isTargetClosed() const { return errorCategory() == CDPErrorCategory::Target; }
    bool isNotFound() const { return errorCategory() == CDPErrorCategory::NotFound; }
    bool isInvalidState() const { return errorCategory() == CDPErrorCategory::InvalidState; }
    bool isProtocolError() const { return errorCategory() == CDPErrorCategory::Protocol; }

    
    bool hasException() const {
        return !hasError && result.contains("exceptionDetails");
    }

    
    std::string exceptionMessage() const {
        if (!hasException()) return "";
        const auto* details = result.find("exceptionDetails");
        if (!details) return "";
        const auto* exc = details->find("exception");
        if (exc) {
            const auto* desc = exc->find("description");
            if (desc && desc->isString()) return desc->asString();
        }
        const auto* text = details->find("text");
        if (text && text->isString()) return text->asString();
        return "Unknown exception";
    }
};

struct CDPEvent {
    std::string method;
    JsonValue params;
};


using ResponseCallback = std::function<void(const CDPResponse&)>;
using EventCallback = std::function<void(const CDPEvent&)>;


class CDPConnection;


enum class ConnectionCallbackType {
    Error,
    Disconnect,
    Reconnecting,
    Reconnected,
    ReconnectFailed
};

class ConnectionCallbackToken {
public:
    ConnectionCallbackToken() = default;

    ConnectionCallbackToken(CDPConnection* conn, ConnectionCallbackType type)
        : connection_(conn), type_(type), active_(true) {}

    
    ConnectionCallbackToken(const ConnectionCallbackToken&) = delete;
    ConnectionCallbackToken& operator=(const ConnectionCallbackToken&) = delete;

    ConnectionCallbackToken(ConnectionCallbackToken&& other) noexcept
        : connection_(other.connection_), type_(other.type_), active_(other.active_) {
        other.connection_ = nullptr;
        other.active_ = false;
    }

    ConnectionCallbackToken& operator=(ConnectionCallbackToken&& other) noexcept {
        if (this != &other) {
            release();
            connection_ = other.connection_;
            type_ = other.type_;
            active_ = other.active_;
            other.connection_ = nullptr;
            other.active_ = false;
        }
        return *this;
    }

    ~ConnectionCallbackToken() {
        release();
    }

    
    void release();

    
    bool isActive() const { return active_; }
    explicit operator bool() const { return active_; }

private:
    CDPConnection* connection_ = nullptr;
    ConnectionCallbackType type_ = ConnectionCallbackType::Error;
    bool active_ = false;
};


enum class ConnectionState {
    Disconnected,       
    Connecting,         
    Connected,          
    Reconnecting        
};


struct ReconnectSettings {
    bool autoReconnect = true;
    int reconnectDelayMs = 1000;
    int reconnectMaxDelayMs = 30000;
    int reconnectMaxAttempts = 0;       
    double backoffMultiplier = 2.0;
    bool enableHeartbeat = true;
    int heartbeatIntervalMs = 15000;
};


class CDPConnection {
public:
    CDPConnection();
    ~CDPConnection();

    
    CDPConnection(const CDPConnection&) = delete;
    CDPConnection& operator=(const CDPConnection&) = delete;
    CDPConnection(CDPConnection&&) = delete;
    CDPConnection& operator=(CDPConnection&&) = delete;

    
    static std::vector<CDPTarget> discoverTargets(const std::string& host = "localhost", int port = 9222);
    static CDPBrowserInfo getBrowserInfo(const std::string& host = "localhost", int port = 9222);
    static std::string getBrowserWebSocketUrl(const std::string& host = "localhost", int port = 9222);

    
    bool connect(const std::string& wsUrl);
    bool connect(const std::string& host, int port, const CDPTarget& target);
    bool connectToTarget(const std::string& host, int port, int targetIndex = 0);
    bool connectToBrowser(const std::string& host = "localhost", int port = 9222);
    void disconnect();

    bool isConnected() const { return ws_.isConnected(); }
    ConnectionState connectionState() const { return connectionState_.load(); }

    
    void setReconnectSettings(const ReconnectSettings& settings) { reconnectSettings_ = settings; }
    const ReconnectSettings& reconnectSettings() const { return reconnectSettings_; }

    
    bool isMessageThread() const {
        return messageThreadRunning_.load() && std::this_thread::get_id() == messageThreadId_;
    }

    
    int64_t sendCommand(const std::string& method, ResponseCallback callback = nullptr);
    int64_t sendCommand(const std::string& method, const JsonValue& params, ResponseCallback callback = nullptr);

    
    CDPResponse sendCommandSync(const std::string& method, int timeoutMs = 30000);
    CDPResponse sendCommandSync(const std::string& method, const JsonValue& params, int timeoutMs = 30000);

    
    void onEvent(const std::string& method, EventCallback callback);
    void onAnyEvent(EventCallback callback);
    void removeEventHandler(const std::string& method);
    void removeEventHandlersByPrefix(const std::string& prefix);  

    
    class EventToken {
    public:
        EventToken() = default;
        EventToken(CDPConnection* conn, const std::string& eventName)
            : connection_(conn), eventName_(eventName), active_(true) {}

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

        ~EventToken() { release(); }

        void release() {
            if (active_ && connection_) {
                connection_->removeEventHandler(eventName_);
                active_ = false;
                connection_ = nullptr;
            }
        }

        bool isActive() const { return active_; }
        explicit operator bool() const { return active_; }

    private:
        CDPConnection* connection_ = nullptr;
        std::string eventName_;
        bool active_ = false;
    };

    [[nodiscard]] EventToken onEventScoped(const std::string& method, EventCallback callback) {
        onEvent(method, std::move(callback));
        return EventToken(this, method);
    }

    
    void poll(int timeoutMs = 0);
    void processMessages(int timeoutMs = 100);

    
    void startMessageThread();
    void stopMessageThread();
    bool isMessageThreadRunning() const { return messageThreadRunning_.load(); }

    
    void stopHeartbeatThread();
    bool isHeartbeatRunning() const { return heartbeatThreadRunning_.load(); }

    
    void stopReconnectThread();
    bool isReconnectThreadRunning() const { return reconnectThreadRunning_.load(); }

    
    void onError(std::function<void(const std::string&)> callback) {
        std::lock_guard<std::mutex> lock(errorCallbackMutex_);
        errorCallback_ = callback;
    }
    void onDisconnect(std::function<void()> callback) {
        std::lock_guard<std::mutex> lock(errorCallbackMutex_);
        disconnectCallback_ = callback;
    }

    
    void onReconnecting(std::function<void(int attempt)> callback) {
        std::lock_guard<std::mutex> lock(errorCallbackMutex_);
        reconnectingCallback_ = callback;
    }
    
    void onReconnected(std::function<void()> callback) {
        std::lock_guard<std::mutex> lock(errorCallbackMutex_);
        reconnectedCallback_ = callback;
    }
    
    void onReconnectFailed(std::function<void(const std::string& reason)> callback) {
        std::lock_guard<std::mutex> lock(errorCallbackMutex_);
        reconnectFailedCallback_ = callback;
    }

    
    [[nodiscard]] ConnectionCallbackToken onErrorScoped(std::function<void(const std::string&)> callback) {
        std::lock_guard<std::mutex> lock(errorCallbackMutex_);
        errorCallback_ = std::move(callback);
        return ConnectionCallbackToken(this, ConnectionCallbackType::Error);
    }

    [[nodiscard]] ConnectionCallbackToken onDisconnectScoped(std::function<void()> callback) {
        std::lock_guard<std::mutex> lock(errorCallbackMutex_);
        disconnectCallback_ = std::move(callback);
        return ConnectionCallbackToken(this, ConnectionCallbackType::Disconnect);
    }

    [[nodiscard]] ConnectionCallbackToken onReconnectingScoped(std::function<void(int)> callback) {
        std::lock_guard<std::mutex> lock(errorCallbackMutex_);
        reconnectingCallback_ = std::move(callback);
        return ConnectionCallbackToken(this, ConnectionCallbackType::Reconnecting);
    }

    [[nodiscard]] ConnectionCallbackToken onReconnectedScoped(std::function<void()> callback) {
        std::lock_guard<std::mutex> lock(errorCallbackMutex_);
        reconnectedCallback_ = std::move(callback);
        return ConnectionCallbackToken(this, ConnectionCallbackType::Reconnected);
    }

    [[nodiscard]] ConnectionCallbackToken onReconnectFailedScoped(std::function<void(const std::string&)> callback) {
        std::lock_guard<std::mutex> lock(errorCallbackMutex_);
        reconnectFailedCallback_ = std::move(callback);
        return ConnectionCallbackToken(this, ConnectionCallbackType::ReconnectFailed);
    }

    
    void clearCallback(ConnectionCallbackType type) {
        std::lock_guard<std::mutex> lock(errorCallbackMutex_);
        switch (type) {
            case ConnectionCallbackType::Error:
                errorCallback_ = nullptr;
                break;
            case ConnectionCallbackType::Disconnect:
                disconnectCallback_ = nullptr;
                break;
            case ConnectionCallbackType::Reconnecting:
                reconnectingCallback_ = nullptr;
                break;
            case ConnectionCallbackType::Reconnected:
                reconnectedCallback_ = nullptr;
                break;
            case ConnectionCallbackType::ReconnectFailed:
                reconnectFailedCallback_ = nullptr;
                break;
        }
    }

    
    int64_t currentMessageId() const { return messageId_.load(); }

private:
    void handleMessage(const std::string& message);
    void handleResponse(const JsonValue& json);
    void handleEvent(const JsonValue& json);
    void messageThreadFunc();
    void heartbeatThreadFunc();
    void attemptReconnect();

    int64_t nextMessageId() { return ++messageId_; }

    WebSocket ws_;
    std::atomic<int64_t> messageId_{0};

    
    std::mutex callbackMutex_;
    std::map<int64_t, ResponseCallback> pendingCallbacks_;
    std::map<int64_t, std::promise<CDPResponse>> pendingPromises_;

    
    mutable std::shared_mutex eventMutex_;
    std::map<std::string, EventCallback> eventHandlers_;
    EventCallback anyEventHandler_;

    
    std::mutex errorCallbackMutex_;
    std::function<void(const std::string&)> errorCallback_;
    std::function<void()> disconnectCallback_;
    std::function<void(int)> reconnectingCallback_;
    std::function<void()> reconnectedCallback_;
    std::function<void(const std::string&)> reconnectFailedCallback_;

    
    std::thread messageThread_;
    std::atomic<bool> messageThreadRunning_{false};
    std::atomic<bool> stopMessageThread_{false};
    std::thread::id messageThreadId_;  

    
    std::thread heartbeatThread_;
    std::atomic<bool> heartbeatThreadRunning_{false};
    std::atomic<bool> stopHeartbeatThread_{false};
    std::chrono::steady_clock::time_point lastActivity_;
    std::mutex activityMutex_;

    
    ReconnectSettings reconnectSettings_;
    std::atomic<ConnectionState> connectionState_{ConnectionState::Disconnected};
    std::string lastWsUrl_;                    
    std::atomic<int> reconnectAttempts_{0};
    std::atomic<bool> intentionalDisconnect_{false};  

    
    std::thread reconnectThread_;
    std::atomic<bool> reconnectThreadRunning_{false};
    std::mutex reconnectMutex_;  

    
    mutable std::mutex eventCondMutex_;
    std::condition_variable eventCond_;
    std::atomic<uint64_t> eventCounter_{0};  

public:
    
    bool waitForEvent(int timeoutMs);

    
    uint64_t eventCount() const { return eventCounter_.load(); }
};

} 
