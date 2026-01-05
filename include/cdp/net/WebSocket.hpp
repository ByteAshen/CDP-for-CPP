

#pragma once

#include "Socket.hpp"
#include "HttpClient.hpp"
#include "../core/Json.hpp"
#include <string>
#include <vector>
#include <functional>
#include <queue>
#include <mutex>
#include <atomic>
#include <random>
#include <shared_mutex>
#include <condition_variable>

namespace cdp {


enum class WebSocketOpcode : uint8_t {
    Continuation = 0x0,
    Text = 0x1,
    Binary = 0x2,
    Close = 0x8,
    Ping = 0x9,
    Pong = 0xA
};


enum class WebSocketCloseCode : uint16_t {
    Normal = 1000,
    GoingAway = 1001,
    ProtocolError = 1002,
    UnsupportedData = 1003,
    NoStatus = 1005,
    Abnormal = 1006,
    InvalidPayload = 1007,
    PolicyViolation = 1008,
    MessageTooBig = 1009,
    MissingExtension = 1010,
    InternalError = 1011,
    ServiceRestart = 1012,
    TryAgainLater = 1013
};


struct WebSocketFrame {
    bool fin = true;
    WebSocketOpcode opcode = WebSocketOpcode::Text;
    bool masked = true;  
    std::vector<uint8_t> payload;

    std::string payloadAsString() const {
        return std::string(payload.begin(), payload.end());
    }
};


enum class WebSocketState {
    Disconnected,
    Connecting,
    Connected,
    Closing,
    Closed
};


struct WebSocketCallbacks {
    std::function<void()> onOpen;
    std::function<void(const std::string&)> onMessage;
    std::function<void(const std::vector<uint8_t>&)> onBinaryMessage;
    std::function<void(WebSocketCloseCode, const std::string&)> onClose;
    std::function<void(const std::string&)> onError;
    std::function<void()> onPong;
};


class WebSocket {
public:
    WebSocket();
    ~WebSocket();

    
    bool connect(const std::string& url);
    bool connect(const std::string& host, int port, const std::string& path);
    void close(WebSocketCloseCode code = WebSocketCloseCode::Normal,
               const std::string& reason = "");
    void disconnect();

    
    WebSocketState state() const { return state_.load(); }
    bool isConnected() const { return state_.load() == WebSocketState::Connected; }

    
    bool send(const std::string& message);
    bool sendBinary(const std::vector<uint8_t>& data);
    bool sendBinary(const uint8_t* data, size_t len);
    bool ping(const std::string& data = "");

    
    bool poll(int timeoutMs = 0);

    
    int pollAll(int initialTimeoutMs = 0);

    bool hasMessage() const {
        std::lock_guard<std::mutex> lock(queueMutex_);
        return !messageQueue_.empty();
    }
    std::string receiveMessage();
    std::vector<uint8_t> receiveBinaryMessage();

    
    void setCallbacks(const WebSocketCallbacks& callbacks) {
        std::lock_guard<std::mutex> lock(callbackMutex_);
        callbacks_ = callbacks;
    }
    void onOpen(std::function<void()> cb) {
        std::lock_guard<std::mutex> lock(callbackMutex_);
        callbacks_.onOpen = cb;
    }
    void onMessage(std::function<void(const std::string&)> cb) {
        std::lock_guard<std::mutex> lock(callbackMutex_);
        callbacks_.onMessage = cb;
    }
    void onBinaryMessage(std::function<void(const std::vector<uint8_t>&)> cb) {
        std::lock_guard<std::mutex> lock(callbackMutex_);
        callbacks_.onBinaryMessage = cb;
    }
    void onClose(std::function<void(WebSocketCloseCode, const std::string&)> cb) {
        std::lock_guard<std::mutex> lock(callbackMutex_);
        callbacks_.onClose = cb;
    }
    void onError(std::function<void(const std::string&)> cb) {
        std::lock_guard<std::mutex> lock(callbackMutex_);
        callbacks_.onError = cb;
    }

    
    void setMaxMessageSize(size_t size) { maxMessageSize_ = size; }

private:
    bool performHandshake(const std::string& host, int port, const std::string& path);
    std::string generateSecKey();
    std::string computeAcceptKey(const std::string& secKey);

    bool sendFrameInternal(WebSocketOpcode opcode, const uint8_t* data, size_t len);
    bool sendFrame(const WebSocketFrame& frame);
    bool sendFrame(WebSocketOpcode opcode, const uint8_t* data, size_t len);
    bool receiveFrame(WebSocketFrame& frame);

    void handleFrame(const WebSocketFrame& frame);
    void handleControlFrame(const WebSocketFrame& frame);

    std::vector<uint8_t> maskData(const uint8_t* data, size_t len);
    uint32_t generateMaskKey();  

    
    template<typename Func, typename... Args>
    void invokeCallback(Func& func, Args&&... args) {
        std::function<void(Args...)> cb;
        {
            std::lock_guard<std::mutex> lock(callbackMutex_);
            cb = func;
        }
        if (cb) cb(std::forward<Args>(args)...);
    }

    TcpSocket socket_;
    std::atomic<WebSocketState> state_{WebSocketState::Disconnected};

    
    mutable std::mutex callbackMutex_;
    WebSocketCallbacks callbacks_;

    
    std::queue<std::string> messageQueue_;
    std::queue<std::vector<uint8_t>> binaryQueue_;
    mutable std::mutex queueMutex_;

    
    std::mutex sendMutex_;
    std::mutex recvMutex_;

    
    std::vector<uint8_t> fragmentBuffer_;
    WebSocketOpcode fragmentOpcode_;

    size_t maxMessageSize_ = 64 * 1024 * 1024;  

    
    static thread_local std::mt19937 tlsRng_;
    static thread_local bool tlsRngInitialized_;
    uint32_t generateMaskKeyFast();
};

} 
