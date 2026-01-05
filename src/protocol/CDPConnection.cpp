

#include "cdp/protocol/CDPConnection.hpp"
#include <sstream>

namespace cdp {


void ConnectionCallbackToken::release() {
    if (active_ && connection_) {
        connection_->clearCallback(type_);
        active_ = false;
        connection_ = nullptr;
    }
}


CDPTarget CDPTarget::fromJson(const JsonValue& json) {
    CDPTarget target;
    target.id = json["id"].getString();
    target.type = json["type"].getString();
    target.title = json["title"].getString();
    target.url = json["url"].getString();
    target.webSocketDebuggerUrl = json["webSocketDebuggerUrl"].getString();
    target.devtoolsFrontendUrl = json["devtoolsFrontendUrl"].getString();
    target.faviconUrl = json["faviconUrl"].getString();
    target.description = json["description"].getString();
    return target;
}


CDPBrowserInfo CDPBrowserInfo::fromJson(const JsonValue& json) {
    CDPBrowserInfo info;
    info.browser = json["Browser"].getString();
    info.protocolVersion = json["Protocol-Version"].getString();
    info.userAgent = json["User-Agent"].getString();
    info.v8Version = json["V8-Version"].getString();
    info.webKitVersion = json["WebKit-Version"].getString();
    return info;
}


std::string CDPRequest::serialize() const {
    JsonObject obj;
    obj["id"] = static_cast<double>(id);
    obj["method"] = method;
    if (!params.isNull()) {
        obj["params"] = params;
    }
    return JsonValue(obj).serialize();
}


CDPConnection::CDPConnection() {
    lastActivity_ = std::chrono::steady_clock::now();

    ws_.onMessage([this](const std::string& message) {
        
        {
            std::lock_guard<std::mutex> lock(activityMutex_);
            lastActivity_ = std::chrono::steady_clock::now();
        }
        handleMessage(message);
    });

    ws_.onClose([this](WebSocketCloseCode code, const std::string& reason) {
        
        auto prevState = connectionState_.exchange(ConnectionState::Disconnected);

        
        if (prevState == ConnectionState::Connected && !intentionalDisconnect_.load()) {
            
            if (reconnectSettings_.autoReconnect && !lastWsUrl_.empty()) {
                connectionState_ = ConnectionState::Reconnecting;
                
                
                {
                    std::lock_guard<std::mutex> lock(reconnectMutex_);
                    
                    if (reconnectThread_.joinable()) {
                        reconnectThread_.join();
                    }
                    reconnectThreadRunning_ = true;
                    reconnectThread_ = std::thread(&CDPConnection::attemptReconnect, this);
                }
                return;  
            }
        }

        
        std::function<void()> cb;
        {
            std::lock_guard<std::mutex> lock(errorCallbackMutex_);
            cb = disconnectCallback_;
        }
        if (cb) cb();
    });

    ws_.onError([this](const std::string& error) {
        std::function<void(const std::string&)> cb;
        {
            std::lock_guard<std::mutex> lock(errorCallbackMutex_);
            cb = errorCallback_;
        }
        if (cb) cb(error);
    });
}

CDPConnection::~CDPConnection() {
    intentionalDisconnect_ = true;  
    stopReconnectThread();  
    stopHeartbeatThread();
    stopMessageThread();
    disconnect();
}

std::vector<CDPTarget> CDPConnection::discoverTargets(const std::string& host, int port) {
    std::vector<CDPTarget> targets;

    HttpClient http;
    if (!http.connect(host, port)) {
        return targets;
    }

    auto response = http.get("/json/list");
    if (!response.isSuccess()) {
        return targets;
    }

    try {
        JsonValue json = JsonValue::parse(response.body);
        if (json.isArray()) {
            for (size_t i = 0; i < json.size(); ++i) {
                targets.push_back(CDPTarget::fromJson(json[i]));
            }
        }
    } catch (const std::exception& e) {
        
        
        (void)e;  
    }

    return targets;
}

CDPBrowserInfo CDPConnection::getBrowserInfo(const std::string& host, int port) {
    HttpClient http;
    if (!http.connect(host, port)) {
        return {};
    }

    auto response = http.get("/json/version");
    if (!response.isSuccess()) {
        return {};
    }

    try {
        JsonValue json = JsonValue::parse(response.body);
        return CDPBrowserInfo::fromJson(json);
    } catch (const std::exception& e) {
        
        (void)e;  
        return {};
    }
}

std::string CDPConnection::getBrowserWebSocketUrl(const std::string& host, int port) {
    HttpClient http;
    if (!http.connect(host, port)) {
        return "";
    }

    auto response = http.get("/json/version");
    if (!response.isSuccess()) {
        return "";
    }

    try {
        JsonValue json = JsonValue::parse(response.body);
        return json["webSocketDebuggerUrl"].getString();
    } catch (const std::exception& e) {
        
        (void)e;  
        return "";
    }
}

bool CDPConnection::connect(const std::string& wsUrl) {
    intentionalDisconnect_ = false;
    connectionState_ = ConnectionState::Connecting;

    if (!ws_.connect(wsUrl)) {
        connectionState_ = ConnectionState::Disconnected;
        return false;
    }

    
    lastWsUrl_ = wsUrl;
    connectionState_ = ConnectionState::Connected;
    reconnectAttempts_ = 0;

    
    if (reconnectSettings_.enableHeartbeat && !heartbeatThreadRunning_.load()) {
        stopHeartbeatThread_ = false;
        heartbeatThreadRunning_ = true;
        heartbeatThread_ = std::thread(&CDPConnection::heartbeatThreadFunc, this);
    }

    
    {
        std::lock_guard<std::mutex> lock(activityMutex_);
        lastActivity_ = std::chrono::steady_clock::now();
    }

    return true;
}

bool CDPConnection::connect(const std::string& host, int port, const CDPTarget& target) {
    if (target.webSocketDebuggerUrl.empty()) {
        std::function<void(const std::string&)> cb;
        {
            std::lock_guard<std::mutex> lock(errorCallbackMutex_);
            cb = errorCallback_;
        }
        if (cb) cb("Target has no WebSocket URL");
        return false;
    }

    
    std::string wsUrl = target.webSocketDebuggerUrl;

    
    auto parsed = Url::parse(wsUrl);
    if (parsed) {
        
        if ((parsed->scheme == "ws" && parsed->port == 80) ||
            (parsed->scheme == "wss" && parsed->port == 443)) {
            parsed->port = port;
            
            std::ostringstream oss;
            oss << parsed->scheme << "://" << parsed->host << ":" << parsed->port << parsed->path;
            if (!parsed->query.empty()) {
                oss << "?" << parsed->query;
            }
            wsUrl = oss.str();
        }
    }

    return ws_.connect(wsUrl);
}

bool CDPConnection::connectToTarget(const std::string& host, int port, int targetIndex) {
    auto targets = discoverTargets(host, port);
    if (targets.empty()) {
        std::function<void(const std::string&)> cb;
        {
            std::lock_guard<std::mutex> lock(errorCallbackMutex_);
            cb = errorCallback_;
        }
        if (cb) cb("No targets found");
        return false;
    }

    if (targetIndex < 0 || targetIndex >= static_cast<int>(targets.size())) {
        std::function<void(const std::string&)> cb;
        {
            std::lock_guard<std::mutex> lock(errorCallbackMutex_);
            cb = errorCallback_;
        }
        if (cb) cb("Invalid target index");
        return false;
    }

    return connect(host, port, targets[targetIndex]);
}

bool CDPConnection::connectToBrowser(const std::string& host, int port) {
    std::string wsUrl = getBrowserWebSocketUrl(host, port);
    if (wsUrl.empty()) {
        std::function<void(const std::string&)> cb;
        {
            std::lock_guard<std::mutex> lock(errorCallbackMutex_);
            cb = errorCallback_;
        }
        if (cb) cb("Could not get browser WebSocket URL");
        return false;
    }
    return connect(wsUrl);  
}

void CDPConnection::disconnect() {
    
    intentionalDisconnect_ = true;
    connectionState_ = ConnectionState::Disconnected;

    
    if (isMessageThread()) {
        std::function<void(const std::string&)> cb;
        {
            std::lock_guard<std::mutex> lock(errorCallbackMutex_);
            cb = errorCallback_;
        }
        if (cb) {
            cb("FATAL: disconnect() called from message thread - would deadlock. "
               "Use async pattern or call from another thread.");
        }
        
        
        return;
    }

    stopHeartbeatThread();
    stopMessageThread();
    ws_.close();

    
    {
        std::lock_guard<std::mutex> lock(callbackMutex_);
        for (auto& [id, promise] : pendingPromises_) {
            CDPResponse errorResponse;
            errorResponse.hasError = true;
            errorResponse.errorMessage = "Disconnected";
            try {
                promise.set_value(errorResponse);
            } catch (const std::future_error&) {
                
            }
        }
        pendingCallbacks_.clear();
        pendingPromises_.clear();
    }

    
    {
        std::unique_lock<std::shared_mutex> lock(eventMutex_);
        eventHandlers_.clear();
        anyEventHandler_ = nullptr;
    }
}

void CDPConnection::startMessageThread() {
    if (messageThreadRunning_.load()) return;

    stopMessageThread_ = false;
    messageThreadRunning_ = true;
    messageThread_ = std::thread(&CDPConnection::messageThreadFunc, this);
    messageThreadId_ = messageThread_.get_id();  
}

void CDPConnection::stopMessageThread() {
    if (!messageThreadRunning_.load()) return;

    
    if (isMessageThread()) {
        
        stopMessageThread_ = true;
        return;
    }

    stopMessageThread_ = true;
    if (messageThread_.joinable()) {
        messageThread_.join();
    }
    messageThreadRunning_ = false;
}

void CDPConnection::messageThreadFunc() {
    
    int pollTimeoutMs = 1;
    const int minPollMs = 1;
    const int maxPollMs = 50;

    while (!stopMessageThread_.load()) {
        if (isConnected()) {
            
            int processed = ws_.pollAll(pollTimeoutMs);

            if (processed > 0) {
                
                pollTimeoutMs = minPollMs;
            } else {
                
                pollTimeoutMs = (std::min)(pollTimeoutMs * 2, maxPollMs);
            }
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(maxPollMs));
            pollTimeoutMs = minPollMs;  
        }
    }
}

void CDPConnection::stopHeartbeatThread() {
    if (!heartbeatThreadRunning_.load()) return;

    stopHeartbeatThread_ = true;
    if (heartbeatThread_.joinable()) {
        heartbeatThread_.join();
    }
    heartbeatThreadRunning_ = false;
}

void CDPConnection::stopReconnectThread() {
    
    
    std::lock_guard<std::mutex> lock(reconnectMutex_);
    if (reconnectThread_.joinable()) {
        reconnectThread_.join();
    }
    reconnectThreadRunning_ = false;
}

void CDPConnection::heartbeatThreadFunc() {
    while (!stopHeartbeatThread_.load()) {
        
        int sleepRemaining = reconnectSettings_.heartbeatIntervalMs;
        const int sleepChunk = 100;  

        while (sleepRemaining > 0 && !stopHeartbeatThread_.load()) {
            int sleepTime = (std::min)(sleepRemaining, sleepChunk);
            std::this_thread::sleep_for(std::chrono::milliseconds(sleepTime));
            sleepRemaining -= sleepTime;
        }

        if (stopHeartbeatThread_.load()) break;

        
        if (!isConnected()) continue;

        
        std::chrono::steady_clock::time_point lastAct;
        {
            std::lock_guard<std::mutex> lock(activityMutex_);
            lastAct = lastActivity_;
        }

        auto now = std::chrono::steady_clock::now();
        auto idleMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastAct).count();

        
        if (idleMs >= reconnectSettings_.heartbeatIntervalMs / 2) {
            
            ws_.ping("heartbeat");

            
            {
                std::lock_guard<std::mutex> lock(activityMutex_);
                lastActivity_ = std::chrono::steady_clock::now();
            }
        }
    }
}

void CDPConnection::attemptReconnect() {
    int attempt = 0;
    int delayMs = reconnectSettings_.reconnectDelayMs;

    while (!intentionalDisconnect_.load()) {
        attempt++;
        reconnectAttempts_ = attempt;

        
        if (reconnectSettings_.reconnectMaxAttempts > 0 &&
            attempt > reconnectSettings_.reconnectMaxAttempts) {

            connectionState_ = ConnectionState::Disconnected;

            
            std::function<void(const std::string&)> failedCb;
            std::function<void()> disconnectCb;
            {
                std::lock_guard<std::mutex> lock(errorCallbackMutex_);
                failedCb = reconnectFailedCallback_;
                disconnectCb = disconnectCallback_;
            }
            if (failedCb) {
                failedCb("Max reconnection attempts reached (" +
                         std::to_string(reconnectSettings_.reconnectMaxAttempts) + ")");
            }
            if (disconnectCb) disconnectCb();
            reconnectThreadRunning_ = false;
            return;
        }

        
        std::function<void(int)> reconnectingCb;
        {
            std::lock_guard<std::mutex> lock(errorCallbackMutex_);
            reconnectingCb = reconnectingCallback_;
        }
        if (reconnectingCb) reconnectingCb(attempt);

        
        std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));

        if (intentionalDisconnect_.load()) break;

        
        connectionState_ = ConnectionState::Connecting;
        if (ws_.connect(lastWsUrl_)) {
            
            connectionState_ = ConnectionState::Connected;
            reconnectAttempts_ = 0;

            
            {
                std::lock_guard<std::mutex> lock(activityMutex_);
                lastActivity_ = std::chrono::steady_clock::now();
            }

            
            if (reconnectSettings_.enableHeartbeat && !heartbeatThreadRunning_.load()) {
                stopHeartbeatThread_ = false;
                heartbeatThreadRunning_ = true;
                heartbeatThread_ = std::thread(&CDPConnection::heartbeatThreadFunc, this);
            }

            
            std::function<void()> reconnectedCb;
            {
                std::lock_guard<std::mutex> lock(errorCallbackMutex_);
                reconnectedCb = reconnectedCallback_;
            }
            if (reconnectedCb) reconnectedCb();
            reconnectThreadRunning_ = false;
            return;
        }

        
        delayMs = static_cast<int>(delayMs * reconnectSettings_.backoffMultiplier);
        delayMs = (std::min)(delayMs, reconnectSettings_.reconnectMaxDelayMs);
    }

    
    connectionState_ = ConnectionState::Disconnected;
    reconnectThreadRunning_ = false;
}

int64_t CDPConnection::sendCommand(const std::string& method, ResponseCallback callback) {
    return sendCommand(method, JsonValue(), callback);
}

int64_t CDPConnection::sendCommand(const std::string& method, const JsonValue& params,
                                    ResponseCallback callback) {
    if (!isConnected()) {
        if (callback) {
            CDPResponse errorResponse;
            errorResponse.hasError = true;
            errorResponse.errorMessage = "Not connected";
            callback(errorResponse);
        }
        return -1;
    }

    int64_t id = nextMessageId();

    CDPRequest request;
    request.id = id;
    request.method = method;
    request.params = params;

    if (callback) {
        std::lock_guard<std::mutex> lock(callbackMutex_);
        pendingCallbacks_[id] = callback;
    }

    std::string message = request.serialize();
    if (!ws_.send(message)) {
        std::lock_guard<std::mutex> lock(callbackMutex_);
        pendingCallbacks_.erase(id);
        if (callback) {
            CDPResponse errorResponse;
            errorResponse.hasError = true;
            errorResponse.errorMessage = "Failed to send message";
            callback(errorResponse);
        }
        return -1;
    }

    return id;
}

CDPResponse CDPConnection::sendCommandSync(const std::string& method, int timeoutMs) {
    return sendCommandSync(method, JsonValue(), timeoutMs);
}

CDPResponse CDPConnection::sendCommandSync(const std::string& method, const JsonValue& params,
                                            int timeoutMs) {
    if (!isConnected()) {
        CDPResponse errorResponse;
        errorResponse.hasError = true;
        errorResponse.errorMessage = "Not connected";
        return errorResponse;
    }

    
    if (isMessageThread()) {
        CDPResponse errorResponse;
        errorResponse.hasError = true;
        errorResponse.errorCode = -1;
        errorResponse.errorMessage = "FATAL: sendCommandSync() called from message thread - would deadlock. "
                                     "Use sendCommand() with callback instead.";
        return errorResponse;
    }

    int64_t id = nextMessageId();

    CDPRequest request;
    request.id = id;
    request.method = method;
    request.params = params;

    std::promise<CDPResponse> promise;
    std::future<CDPResponse> future = promise.get_future();

    {
        std::lock_guard<std::mutex> lock(callbackMutex_);
        pendingPromises_[id] = std::move(promise);
    }

    std::string message = request.serialize();
    if (!ws_.send(message)) {
        std::lock_guard<std::mutex> lock(callbackMutex_);
        pendingPromises_.erase(id);
        CDPResponse errorResponse;
        errorResponse.hasError = true;
        errorResponse.errorMessage = "Failed to send message";
        return errorResponse;
    }

    
    if (messageThreadRunning_.load()) {
        auto status = future.wait_for(std::chrono::milliseconds(timeoutMs));
        if (status == std::future_status::ready) {
            return future.get();
        } else {
            std::lock_guard<std::mutex> lock(callbackMutex_);
            pendingPromises_.erase(id);
            CDPResponse errorResponse;
            errorResponse.hasError = true;
            errorResponse.errorMessage = "Timeout waiting for response";
            return errorResponse;
        }
    }

    
    auto startTime = std::chrono::steady_clock::now();
    while (true) {
        auto status = future.wait_for(std::chrono::milliseconds(10));
        if (status == std::future_status::ready) {
            return future.get();
        }

        
        poll(10);

        
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - startTime).count();
        if (elapsed >= timeoutMs) {
            std::lock_guard<std::mutex> lock(callbackMutex_);
            pendingPromises_.erase(id);
            CDPResponse errorResponse;
            errorResponse.hasError = true;
            errorResponse.errorMessage = "Timeout waiting for response";
            return errorResponse;
        }
    }
}

void CDPConnection::onEvent(const std::string& method, EventCallback callback) {
    std::unique_lock<std::shared_mutex> lock(eventMutex_);
    eventHandlers_[method] = callback;
}

void CDPConnection::onAnyEvent(EventCallback callback) {
    std::unique_lock<std::shared_mutex> lock(eventMutex_);
    anyEventHandler_ = callback;
}

void CDPConnection::removeEventHandler(const std::string& method) {
    std::unique_lock<std::shared_mutex> lock(eventMutex_);
    eventHandlers_.erase(method);
}

void CDPConnection::removeEventHandlersByPrefix(const std::string& prefix) {
    std::unique_lock<std::shared_mutex> lock(eventMutex_);
    for (auto it = eventHandlers_.begin(); it != eventHandlers_.end(); ) {
        if (it->first.rfind(prefix, 0) == 0) {  
            it = eventHandlers_.erase(it);
        } else {
            ++it;
        }
    }
}

void CDPConnection::poll(int timeoutMs) {
    ws_.poll(timeoutMs);
}

void CDPConnection::processMessages(int timeoutMs) {
    auto startTime = std::chrono::steady_clock::now();
    while (true) {
        poll(10);

        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - startTime).count();
        if (elapsed >= timeoutMs) break;
    }
}

void CDPConnection::handleMessage(const std::string& message) {
    try {
        JsonValue json = JsonValue::parse(message);

        if (json.contains("id")) {
            handleResponse(json);
        } else if (json.contains("method")) {
            handleEvent(json);
        }
    } catch (const std::exception& e) {
        std::function<void(const std::string&)> cb;
        {
            std::lock_guard<std::mutex> lock(errorCallbackMutex_);
            cb = errorCallback_;
        }
        if (cb) cb(std::string("Failed to parse message: ") + e.what());
    }
}

void CDPConnection::handleResponse(const JsonValue& json) {
    int64_t id = json["id"].asInt64();

    CDPResponse response;
    response.id = id;

    if (json.contains("error")) {
        response.hasError = true;
        response.errorCode = json["error"]["code"].getInt();
        response.errorMessage = json["error"]["message"].getString();
    } else {
        response.result = json["result"];
    }

    ResponseCallback callback;
    std::promise<CDPResponse> promise;
    bool hasPromise = false;

    {
        std::lock_guard<std::mutex> lock(callbackMutex_);

        
        auto promiseIt = pendingPromises_.find(id);
        if (promiseIt != pendingPromises_.end()) {
            promise = std::move(promiseIt->second);
            pendingPromises_.erase(promiseIt);
            hasPromise = true;
        } else {
            
            auto callbackIt = pendingCallbacks_.find(id);
            if (callbackIt != pendingCallbacks_.end()) {
                callback = std::move(callbackIt->second);
                pendingCallbacks_.erase(callbackIt);
            }
        }
    }

    
    if (hasPromise) {
        promise.set_value(std::move(response));
        return;
    }

    
    if (callback) {
        try {
            callback(response);
        } catch (const std::exception& e) {
            std::function<void(const std::string&)> errorCb;
            {
                std::lock_guard<std::mutex> lock(errorCallbackMutex_);
                errorCb = errorCallback_;
            }
            if (errorCb) {
                errorCb(std::string("Response callback exception: ") + e.what());
            }
        } catch (...) {
            std::function<void(const std::string&)> errorCb;
            {
                std::lock_guard<std::mutex> lock(errorCallbackMutex_);
                errorCb = errorCallback_;
            }
            if (errorCb) {
                errorCb("Response callback threw unknown exception");
            }
        }
    }
}

void CDPConnection::handleEvent(const JsonValue& json) {
    CDPEvent event;
    event.method = json["method"].asString();
    event.params = json["params"];

    EventCallback specificHandler;
    EventCallback anyHandler;

    {
        std::shared_lock<std::shared_mutex> lock(eventMutex_);

        
        auto it = eventHandlers_.find(event.method);
        if (it != eventHandlers_.end()) {
            specificHandler = it->second;
        }

        
        anyHandler = anyEventHandler_;
    }

    
    if (specificHandler) {
        try {
            specificHandler(event);
        } catch (const std::exception& e) {
            
            std::function<void(const std::string&)> errorCb;
            {
                std::lock_guard<std::mutex> lock(errorCallbackMutex_);
                errorCb = errorCallback_;
            }
            if (errorCb) {
                errorCb(std::string("Event handler exception: ") + e.what());
            }
        } catch (...) {
            
            std::function<void(const std::string&)> errorCb;
            {
                std::lock_guard<std::mutex> lock(errorCallbackMutex_);
                errorCb = errorCallback_;
            }
            if (errorCb) {
                errorCb("Event handler threw unknown exception");
            }
        }
    }
    if (anyHandler) {
        try {
            anyHandler(event);
        } catch (const std::exception& e) {
            std::function<void(const std::string&)> errorCb;
            {
                std::lock_guard<std::mutex> lock(errorCallbackMutex_);
                errorCb = errorCallback_;
            }
            if (errorCb) {
                errorCb(std::string("Any-event handler exception: ") + e.what());
            }
        } catch (...) {
            std::function<void(const std::string&)> errorCb;
            {
                std::lock_guard<std::mutex> lock(errorCallbackMutex_);
                errorCb = errorCallback_;
            }
            if (errorCb) {
                errorCb("Any-event handler threw unknown exception");
            }
        }
    }

    
    {
        std::lock_guard<std::mutex> lock(eventCondMutex_);
        eventCounter_++;
    }
    eventCond_.notify_all();
}

bool CDPConnection::waitForEvent(int timeoutMs) {
    uint64_t countBefore = eventCounter_.load();

    std::unique_lock<std::mutex> lock(eventCondMutex_);
    return eventCond_.wait_for(lock, std::chrono::milliseconds(timeoutMs),
        [this, countBefore]() { return eventCounter_.load() > countBefore; });
}

} 
