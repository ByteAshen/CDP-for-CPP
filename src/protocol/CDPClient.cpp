

#include "cdp/protocol/CDPClient.hpp"
#include <thread>

namespace cdp {

CDPClient::CDPClient() : CDPClient(CDPClientConfig{}) {}

CDPClient::CDPClient(const CDPClientConfig& config) : config_(config) {
    
    std::string validationError = config_.validate();
    if (!validationError.empty()) {
        lastError_ = "Invalid configuration: " + validationError;
        
        
    }

    
    ReconnectSettings reconnectSettings;
    reconnectSettings.autoReconnect = config_.autoReconnect;
    reconnectSettings.reconnectDelayMs = config_.reconnectDelayMs;
    reconnectSettings.reconnectMaxDelayMs = config_.reconnectMaxDelayMs;
    reconnectSettings.reconnectMaxAttempts = config_.reconnectMaxAttempts;
    reconnectSettings.backoffMultiplier = config_.reconnectBackoffMultiplier;
    reconnectSettings.enableHeartbeat = config_.enableHeartbeat;
    reconnectSettings.heartbeatIntervalMs = config_.heartbeatIntervalMs;
    connection_.setReconnectSettings(reconnectSettings);
}

CDPClient::~CDPClient() {
    disconnect();
}

void CDPClient::enableDomains() {
    if (!config_.autoEnableDomains) return;

    Page.enable();
    Runtime.enable();
    DOM.enable();
    Network.enable();

    
    Page.onLoadEventFired([this](double) {
        pageLoaded_.store(true);
    });
}

bool CDPClient::connect(int targetIndex) {
    lastError_.clear();

    
    if (!config_.isValid()) {
        lastError_ = "Invalid configuration: " + config_.validate();
        return false;
    }

    
    auto targets = listTargets();
    if (targets.empty()) {
        lastError_ = "No targets found. Is Chrome running with --remote-debugging-port=" +
                     std::to_string(config_.port) + "?";
        return false;
    }

    
    int actualIndex = targetIndex;
    if (targetIndex == 0) {
        for (size_t i = 0; i < targets.size(); ++i) {
            if (targets[i].type == "page") {
                actualIndex = static_cast<int>(i);
                break;
            }
        }
    }

    if (actualIndex < 0 || actualIndex >= static_cast<int>(targets.size())) {
        lastError_ = "Target index " + std::to_string(actualIndex) + " out of range (0-" +
                     std::to_string(targets.size() - 1) + ")";
        return false;
    }

    bool result = connection_.connectToTarget(config_.host, config_.port, actualIndex);
    if (!result) {
        lastError_ = "Failed to connect to target at " + config_.host + ":" +
                     std::to_string(config_.port);
        return false;
    }

    if (config_.useBackgroundThread) {
        connection_.startMessageThread();
    }
    enableDomains();
    return true;
}

bool CDPClient::connect(const std::string& webSocketUrl) {
    lastError_.clear();

    if (webSocketUrl.empty()) {
        lastError_ = "WebSocket URL cannot be empty";
        return false;
    }

    bool result = connection_.connect(webSocketUrl);
    if (!result) {
        lastError_ = "Failed to connect to WebSocket URL: " + webSocketUrl;
        return false;
    }

    if (config_.useBackgroundThread) {
        connection_.startMessageThread();
    }
    enableDomains();
    return true;
}

bool CDPClient::connectToTarget(const CDPTarget& target) {
    lastError_.clear();

    if (target.webSocketDebuggerUrl.empty()) {
        lastError_ = "Target has no WebSocket URL (target id: " + target.id + ")";
        return false;
    }

    bool result = connection_.connect(config_.host, config_.port, target);
    if (!result) {
        lastError_ = "Failed to connect to target: " + target.title + " (id: " + target.id + ")";
        return false;
    }

    if (config_.useBackgroundThread) {
        connection_.startMessageThread();
    }
    enableDomains();
    return true;
}

bool CDPClient::connectToBrowser() {
    lastError_.clear();

    
    if (!config_.isValid()) {
        lastError_ = "Invalid configuration: " + config_.validate();
        return false;
    }

    bool result = connection_.connectToBrowser(config_.host, config_.port);
    if (!result) {
        lastError_ = "Failed to connect to browser at " + config_.host + ":" +
                     std::to_string(config_.port);
        return false;
    }

    if (config_.useBackgroundThread) {
        connection_.startMessageThread();
    }
    return true;
}

void CDPClient::disconnect() {
    connection_.disconnect();
    pageLoaded_.store(false);
}

bool CDPClient::isConnected() const {
    return connection_.isConnected();
}

const std::string& CDPClient::lastError() const {
    return lastError_;
}

std::vector<CDPTarget> CDPClient::listTargets() {
    return CDPConnection::discoverTargets(config_.host, config_.port);
}

CDPBrowserInfo CDPClient::getBrowserInfo() {
    return CDPConnection::getBrowserInfo(config_.host, config_.port);
}

CDPResponse CDPClient::sendCommand(const std::string& method, const JsonValue& params) {
    return connection_.sendCommandSync(method, params, config_.commandTimeoutMs);
}

void CDPClient::poll(int timeoutMs) {
    
    
    if (connection_.isMessageThreadRunning()) {
        connection_.waitForEvent(timeoutMs);
    } else {
        connection_.processMessages(timeoutMs);
    }
}

bool CDPClient::waitFor(std::function<bool()> condition, int timeoutMs, int pollIntervalMs) {
    auto startTime = std::chrono::steady_clock::now();
    while (true) {
        
        if (condition()) return true;

        
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - startTime).count();
        if (elapsed >= timeoutMs) return false;

        
        int remaining = static_cast<int>(timeoutMs - elapsed);
        int waitTime = (remaining < pollIntervalMs) ? remaining : pollIntervalMs;

        
        if (connection_.isMessageThreadRunning()) {
            connection_.waitForEvent(waitTime);
        } else {
            connection_.processMessages(waitTime);
        }
    }
}

void CDPClient::sleep(int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

} 
