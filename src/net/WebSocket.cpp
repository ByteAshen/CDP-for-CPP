

#include "cdp/net/WebSocket.hpp"
#include "cdp/core/Base64.hpp"
#include "cdp/core/SHA1.hpp"
#include <sstream>
#include <algorithm>
#include <cstring>

namespace cdp {

namespace {
    const std::string WS_MAGIC_GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
}


thread_local std::mt19937 WebSocket::tlsRng_;
thread_local bool WebSocket::tlsRngInitialized_ = false;

WebSocket::WebSocket() {}

WebSocket::~WebSocket() {
    if (state_.load() == WebSocketState::Connected) {
        close();
    }
    disconnect();
}

bool WebSocket::connect(const std::string& url) {
    auto parsedUrl = Url::parse(url);
    if (!parsedUrl) {
        std::function<void(const std::string&)> errorCb;
        {
            std::lock_guard<std::mutex> lock(callbackMutex_);
            errorCb = callbacks_.onError;
        }
        if (errorCb) errorCb("Invalid URL: " + url);
        return false;
    }

    
    std::string scheme = parsedUrl->scheme;
    if (scheme != "ws" && scheme != "wss" && scheme != "http" && scheme != "https") {
        std::function<void(const std::string&)> errorCb;
        {
            std::lock_guard<std::mutex> lock(callbackMutex_);
            errorCb = callbacks_.onError;
        }
        if (errorCb) errorCb("Invalid scheme: " + scheme);
        return false;
    }

    int port = parsedUrl->port;
    if (port == 80 && (scheme == "ws" || scheme == "http")) port = 80;
    else if (port == 443 && (scheme == "wss" || scheme == "https")) port = 443;
    else if (parsedUrl->port == 80 && scheme == "wss") port = 443;

    std::string path = parsedUrl->path;
    if (path.empty()) path = "/";
    if (!parsedUrl->query.empty()) path += "?" + parsedUrl->query;

    return connect(parsedUrl->host, port, path);
}

bool WebSocket::connect(const std::string& host, int port, const std::string& path) {
    if (state_.load() != WebSocketState::Disconnected) {
        disconnect();
    }

    state_ = WebSocketState::Connecting;

    socket_ = TcpSocket();
    socket_.setTimeout(30000, 30000);
    socket_.setNoDelay(true);

    if (!socket_.connect(host, port)) {
        state_ = WebSocketState::Disconnected;
        std::function<void(const std::string&)> errorCb;
        {
            std::lock_guard<std::mutex> lock(callbackMutex_);
            errorCb = callbacks_.onError;
        }
        if (errorCb) errorCb("Failed to connect to " + host + ":" + std::to_string(port));
        return false;
    }

    if (!performHandshake(host, port, path)) {
        disconnect();
        return false;
    }

    state_ = WebSocketState::Connected;

    std::function<void()> openCb;
    {
        std::lock_guard<std::mutex> lock(callbackMutex_);
        openCb = callbacks_.onOpen;
    }
    if (openCb) openCb();

    return true;
}

bool WebSocket::performHandshake(const std::string& host, int port, const std::string& path) {
    std::string secKey = generateSecKey();
    std::string expectedAccept = computeAcceptKey(secKey);

    
    std::ostringstream request;
    request << "GET " << path << " HTTP/1.1\r\n";
    request << "Host: " << host;
    if ((port != 80 && port != 443)) {
        request << ":" << port;
    }
    request << "\r\n";
    request << "Upgrade: websocket\r\n";
    request << "Connection: Upgrade\r\n";
    request << "Sec-WebSocket-Key: " << secKey << "\r\n";
    request << "Sec-WebSocket-Version: 13\r\n";
    request << "\r\n";

    std::string reqStr = request.str();
    if (socket_.send(reqStr) < 0) {
        std::function<void(const std::string&)> errorCb;
        {
            std::lock_guard<std::mutex> lock(callbackMutex_);
            errorCb = callbacks_.onError;
        }
        if (errorCb) errorCb("Failed to send handshake request");
        return false;
    }

    
    std::string response = socket_.recvUntil("\r\n\r\n", 30000);
    if (response.empty()) {
        std::function<void(const std::string&)> errorCb;
        {
            std::lock_guard<std::mutex> lock(callbackMutex_);
            errorCb = callbacks_.onError;
        }
        if (errorCb) errorCb("No handshake response received");
        return false;
    }

    
    std::istringstream responseStream(response);
    std::string statusLine;
    std::getline(responseStream, statusLine);

    
    if (statusLine.find("101") == std::string::npos) {
        std::function<void(const std::string&)> errorCb;
        {
            std::lock_guard<std::mutex> lock(callbackMutex_);
            errorCb = callbacks_.onError;
        }
        if (errorCb) errorCb("Handshake failed: " + statusLine);
        return false;
    }

    
    std::map<std::string, std::string> headers;
    std::string line;
    while (std::getline(responseStream, line) && line != "\r" && !line.empty()) {
        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos) {
            std::string key = line.substr(0, colonPos);

            
            size_t valueStart = colonPos + 1;
            while (valueStart < line.size() && (line[valueStart] == ' ' || line[valueStart] == '\t')) {
                valueStart++;
            }
            
            size_t valueEnd = line.size();
            while (valueEnd > valueStart && (line[valueEnd - 1] == '\r' || line[valueEnd - 1] == '\n')) {
                valueEnd--;
            }
            std::string value = line.substr(valueStart, valueEnd - valueStart);

            
            std::transform(key.begin(), key.end(), key.begin(),
                [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            headers[key] = std::move(value);
        }
    }

    
    auto upgradeIt = headers.find("upgrade");
    if (upgradeIt == headers.end()) {
        std::function<void(const std::string&)> errorCb;
        {
            std::lock_guard<std::mutex> lock(callbackMutex_);
            errorCb = callbacks_.onError;
        }
        if (errorCb) errorCb("Missing Upgrade header");
        return false;
    }
    std::string upgradeValue = upgradeIt->second;
    std::transform(upgradeValue.begin(), upgradeValue.end(), upgradeValue.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (upgradeValue != "websocket") {
        std::function<void(const std::string&)> errorCb;
        {
            std::lock_guard<std::mutex> lock(callbackMutex_);
            errorCb = callbacks_.onError;
        }
        if (errorCb) errorCb("Invalid Upgrade header: " + upgradeIt->second);
        return false;
    }

    auto acceptIt = headers.find("sec-websocket-accept");
    if (acceptIt == headers.end()) {
        std::function<void(const std::string&)> errorCb;
        {
            std::lock_guard<std::mutex> lock(callbackMutex_);
            errorCb = callbacks_.onError;
        }
        if (errorCb) errorCb("Missing Sec-WebSocket-Accept header");
        return false;
    }

    if (acceptIt->second != expectedAccept) {
        std::function<void(const std::string&)> errorCb;
        {
            std::lock_guard<std::mutex> lock(callbackMutex_);
            errorCb = callbacks_.onError;
        }
        if (errorCb) errorCb("Invalid Sec-WebSocket-Accept");
        return false;
    }

    return true;
}

std::string WebSocket::generateSecKey() {
    uint8_t bytes[16];
    for (int i = 0; i < 16; ++i) {
        bytes[i] = static_cast<uint8_t>(generateMaskKeyFast() & 0xFF);
    }
    return Base64::encode(bytes, 16);
}

std::string WebSocket::computeAcceptKey(const std::string& secKey) {
    std::string concat = secKey + WS_MAGIC_GUID;
    auto hash = SHA1::hash(concat);
    return Base64::encode(hash.data(), hash.size());
}

void WebSocket::close(WebSocketCloseCode code, const std::string& reason) {
    WebSocketState expected = WebSocketState::Connected;
    if (!state_.compare_exchange_strong(expected, WebSocketState::Closing)) {
        return;  
    }

    
    std::vector<uint8_t> payload;
    uint16_t codeValue = static_cast<uint16_t>(code);
    payload.push_back(static_cast<uint8_t>(codeValue >> 8));
    payload.push_back(static_cast<uint8_t>(codeValue & 0xFF));
    payload.insert(payload.end(), reason.begin(), reason.end());

    sendFrame(WebSocketOpcode::Close, payload.data(), payload.size());

    
    socket_.setTimeout(5000, 5000);
    WebSocketFrame frame;
    {
        std::lock_guard<std::mutex> lock(recvMutex_);
        while (receiveFrame(frame)) {
            if (frame.opcode == WebSocketOpcode::Close) break;
        }
    }

    state_ = WebSocketState::Closed;

    std::function<void(WebSocketCloseCode, const std::string&)> closeCb;
    {
        std::lock_guard<std::mutex> lock(callbackMutex_);
        closeCb = callbacks_.onClose;
    }
    if (closeCb) closeCb(code, reason);
}

void WebSocket::disconnect() {
    if (socket_.isValid()) {
        socket_.disconnect();
    }
    state_ = WebSocketState::Disconnected;

    std::lock_guard<std::mutex> lock(queueMutex_);
    while (!messageQueue_.empty()) messageQueue_.pop();
    while (!binaryQueue_.empty()) binaryQueue_.pop();

    
    fragmentBuffer_.clear();
}

bool WebSocket::send(const std::string& message) {
    return sendFrame(WebSocketOpcode::Text,
                     reinterpret_cast<const uint8_t*>(message.data()),
                     message.size());
}

bool WebSocket::sendBinary(const std::vector<uint8_t>& data) {
    return sendFrame(WebSocketOpcode::Binary, data.data(), data.size());
}

bool WebSocket::sendBinary(const uint8_t* data, size_t len) {
    return sendFrame(WebSocketOpcode::Binary, data, len);
}

bool WebSocket::ping(const std::string& data) {
    return sendFrame(WebSocketOpcode::Ping,
                     reinterpret_cast<const uint8_t*>(data.data()),
                     data.size());
}

bool WebSocket::sendFrame(const WebSocketFrame& frame) {
    return sendFrame(frame.opcode, frame.payload.data(), frame.payload.size());
}

bool WebSocket::sendFrame(WebSocketOpcode opcode, const uint8_t* data, size_t len) {
    std::lock_guard<std::mutex> lock(sendMutex_);
    return sendFrameInternal(opcode, data, len);
}

bool WebSocket::sendFrameInternal(WebSocketOpcode opcode, const uint8_t* data, size_t len) {
    
    auto currentState = state_.load();
    if (currentState != WebSocketState::Connected && currentState != WebSocketState::Closing) {
        return false;
    }

    std::vector<uint8_t> frame;

    
    frame.push_back(0x80 | static_cast<uint8_t>(opcode));

    
    uint32_t maskKey = generateMaskKey();

    if (len < 126) {
        frame.push_back(0x80 | static_cast<uint8_t>(len));
    } else if (len < 65536) {
        frame.push_back(0x80 | 126);
        frame.push_back(static_cast<uint8_t>(len >> 8));
        frame.push_back(static_cast<uint8_t>(len & 0xFF));
    } else {
        frame.push_back(0x80 | 127);
        for (int i = 7; i >= 0; --i) {
            frame.push_back(static_cast<uint8_t>((len >> (i * 8)) & 0xFF));
        }
    }

    
    frame.push_back(static_cast<uint8_t>(maskKey >> 24));
    frame.push_back(static_cast<uint8_t>(maskKey >> 16));
    frame.push_back(static_cast<uint8_t>(maskKey >> 8));
    frame.push_back(static_cast<uint8_t>(maskKey));

    
    uint8_t mask[4] = {
        static_cast<uint8_t>(maskKey >> 24),
        static_cast<uint8_t>(maskKey >> 16),
        static_cast<uint8_t>(maskKey >> 8),
        static_cast<uint8_t>(maskKey)
    };

    for (size_t i = 0; i < len; ++i) {
        frame.push_back(data[i] ^ mask[i % 4]);
    }

    int sent = socket_.send(reinterpret_cast<const char*>(frame.data()),
                            static_cast<int>(frame.size()));
    return sent == static_cast<int>(frame.size());
}

bool WebSocket::receiveFrame(WebSocketFrame& frame) {
    if (!socket_.isConnected()) return false;

    
    std::string header = socket_.recvExact(2, 30000);
    if (header.size() < 2) return false;

    uint8_t byte1 = static_cast<uint8_t>(header[0]);
    uint8_t byte2 = static_cast<uint8_t>(header[1]);

    frame.fin = (byte1 & 0x80) != 0;
    frame.opcode = static_cast<WebSocketOpcode>(byte1 & 0x0F);
    frame.masked = (byte2 & 0x80) != 0;

    uint64_t payloadLen = byte2 & 0x7F;

    if (payloadLen == 126) {
        std::string lenBytes = socket_.recvExact(2, 30000);
        if (lenBytes.size() < 2) return false;
        payloadLen = (static_cast<uint64_t>(static_cast<uint8_t>(lenBytes[0])) << 8) |
                     static_cast<uint64_t>(static_cast<uint8_t>(lenBytes[1]));
    } else if (payloadLen == 127) {
        std::string lenBytes = socket_.recvExact(8, 30000);
        if (lenBytes.size() < 8) return false;
        payloadLen = 0;
        for (int i = 0; i < 8; ++i) {
            payloadLen = (payloadLen << 8) | static_cast<uint64_t>(static_cast<uint8_t>(lenBytes[i]));
        }
    }

    if (payloadLen > maxMessageSize_) {
        if (callbacks_.onError) callbacks_.onError("Message too large");
        return false;
    }

    
    uint8_t mask[4] = {0};
    if (frame.masked) {
        std::string maskBytes = socket_.recvExact(4, 30000);
        if (maskBytes.size() < 4) return false;
        for (int i = 0; i < 4; ++i) {
            mask[i] = static_cast<uint8_t>(maskBytes[i]);
        }
    }

    
    frame.payload.clear();
    if (payloadLen > 0) {
        std::string payloadData = socket_.recvExact(static_cast<int>(payloadLen), 30000);
        if (payloadData.size() < payloadLen) return false;

        frame.payload.resize(payloadLen);
        for (size_t i = 0; i < payloadLen; ++i) {
            uint8_t byte = static_cast<uint8_t>(payloadData[i]);
            if (frame.masked) {
                byte ^= mask[i % 4];
            }
            frame.payload[i] = byte;
        }
    }

    return true;
}

bool WebSocket::poll(int timeoutMs) {
    if (state_.load() != WebSocketState::Connected) return false;

    if (!socket_.hasData(timeoutMs)) return false;

    std::lock_guard<std::mutex> lock(recvMutex_);

    WebSocketFrame frame;
    if (!receiveFrame(frame)) {
        state_ = WebSocketState::Disconnected;
        std::function<void(WebSocketCloseCode, const std::string&)> closeCb;
        {
            std::lock_guard<std::mutex> cbLock(callbackMutex_);
            closeCb = callbacks_.onClose;
        }
        if (closeCb) closeCb(WebSocketCloseCode::Abnormal, "Connection lost");
        return false;
    }

    handleFrame(frame);
    return true;
}

int WebSocket::pollAll(int initialTimeoutMs) {
    if (state_.load() != WebSocketState::Connected) return 0;

    
    if (!socket_.hasData(initialTimeoutMs)) return 0;

    std::lock_guard<std::mutex> lock(recvMutex_);

    int count = 0;

    
    while (socket_.hasData(0)) {  
        if (state_.load() != WebSocketState::Connected) break;

        WebSocketFrame frame;
        if (!receiveFrame(frame)) {
            state_ = WebSocketState::Disconnected;
            std::function<void(WebSocketCloseCode, const std::string&)> closeCb;
            {
                std::lock_guard<std::mutex> cbLock(callbackMutex_);
                closeCb = callbacks_.onClose;
            }
            if (closeCb) closeCb(WebSocketCloseCode::Abnormal, "Connection lost");
            break;
        }

        handleFrame(frame);
        count++;

        
        constexpr int kMaxFramesPerPoll = 10000;
        if (count >= kMaxFramesPerPoll) break;
    }

    return count;
}

void WebSocket::handleFrame(const WebSocketFrame& frame) {
    

    if (static_cast<uint8_t>(frame.opcode) >= 0x8) {
        handleControlFrame(frame);
        return;
    }

    
    if (frame.opcode == WebSocketOpcode::Continuation) {
        fragmentBuffer_.insert(fragmentBuffer_.end(),
                               frame.payload.begin(), frame.payload.end());
    } else {
        fragmentBuffer_ = frame.payload;
        fragmentOpcode_ = frame.opcode;
    }

    if (frame.fin) {
        
        std::function<void(const std::string&)> msgCb;
        std::function<void(const std::vector<uint8_t>&)> binCb;
        {
            std::lock_guard<std::mutex> cbLock(callbackMutex_);
            if (fragmentOpcode_ == WebSocketOpcode::Text) {
                msgCb = callbacks_.onMessage;
            } else if (fragmentOpcode_ == WebSocketOpcode::Binary) {
                binCb = callbacks_.onBinaryMessage;
            }
        }

        if (fragmentOpcode_ == WebSocketOpcode::Text) {
            
            std::string message(fragmentBuffer_.begin(), fragmentBuffer_.end());
            fragmentBuffer_.clear();  

            {
                std::lock_guard<std::mutex> lock(queueMutex_);
                messageQueue_.push(message);  
            }
            if (msgCb) msgCb(std::move(message));  
        } else if (fragmentOpcode_ == WebSocketOpcode::Binary) {
            
            std::vector<uint8_t> data = std::move(fragmentBuffer_);

            {
                std::lock_guard<std::mutex> lock(queueMutex_);
                binaryQueue_.push(data);  
            }
            if (binCb) binCb(std::move(data));  
        } else {
            fragmentBuffer_.clear();
        }
    }
}

void WebSocket::handleControlFrame(const WebSocketFrame& frame) {
    

    switch (frame.opcode) {
        case WebSocketOpcode::Close: {
            WebSocketCloseCode code = WebSocketCloseCode::NoStatus;
            std::string reason;

            if (frame.payload.size() >= 2) {
                code = static_cast<WebSocketCloseCode>(
                    (static_cast<uint16_t>(frame.payload[0]) << 8) |
                    static_cast<uint16_t>(frame.payload[1]));

                if (frame.payload.size() > 2) {
                    reason = std::string(frame.payload.begin() + 2, frame.payload.end());
                }
            }

            if (state_.load() == WebSocketState::Connected) {
                
                std::lock_guard<std::mutex> sendLock(sendMutex_);
                sendFrameInternal(WebSocketOpcode::Close, frame.payload.data(), frame.payload.size());
            }

            state_ = WebSocketState::Closed;

            std::function<void(WebSocketCloseCode, const std::string&)> closeCb;
            {
                std::lock_guard<std::mutex> cbLock(callbackMutex_);
                closeCb = callbacks_.onClose;
            }
            if (closeCb) closeCb(code, reason);
            break;
        }

        case WebSocketOpcode::Ping: {
            
            std::lock_guard<std::mutex> sendLock(sendMutex_);
            sendFrameInternal(WebSocketOpcode::Pong, frame.payload.data(), frame.payload.size());
            break;
        }

        case WebSocketOpcode::Pong: {
            std::function<void()> pongCb;
            {
                std::lock_guard<std::mutex> cbLock(callbackMutex_);
                pongCb = callbacks_.onPong;
            }
            if (pongCb) pongCb();
            break;
        }

        default:
            break;
    }
}

std::string WebSocket::receiveMessage() {
    std::lock_guard<std::mutex> lock(queueMutex_);
    if (messageQueue_.empty()) return "";
    std::string msg = std::move(messageQueue_.front());
    messageQueue_.pop();
    return msg;
}

std::vector<uint8_t> WebSocket::receiveBinaryMessage() {
    std::lock_guard<std::mutex> lock(queueMutex_);
    if (binaryQueue_.empty()) return {};
    std::vector<uint8_t> msg = std::move(binaryQueue_.front());
    binaryQueue_.pop();
    return msg;
}

uint32_t WebSocket::generateMaskKey() {
    
    return generateMaskKeyFast();
}

uint32_t WebSocket::generateMaskKeyFast() {
    
    if (!tlsRngInitialized_) {
        std::random_device rd;
        tlsRng_.seed(rd());
        tlsRngInitialized_ = true;
    }
    return tlsRng_();
}

std::vector<uint8_t> WebSocket::maskData(const uint8_t* data, size_t len) {
    uint32_t maskKey = generateMaskKey();
    uint8_t mask[4] = {
        static_cast<uint8_t>(maskKey >> 24),
        static_cast<uint8_t>(maskKey >> 16),
        static_cast<uint8_t>(maskKey >> 8),
        static_cast<uint8_t>(maskKey)
    };

    std::vector<uint8_t> result(len);
    for (size_t i = 0; i < len; ++i) {
        result[i] = data[i] ^ mask[i % 4];
    }
    return result;
}

} 
