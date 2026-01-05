

#pragma once

#include "CDPConnection.hpp"
#include "../domains/Domain.hpp"
#include "../domains/Page.hpp"
#include "../domains/Runtime.hpp"
#include "../domains/DOM.hpp"
#include "../domains/Network.hpp"
#include "../domains/Input.hpp"
#include "../domains/Emulation.hpp"
#include "../domains/Target.hpp"
#include "../domains/Browser.hpp"
#include "../domains/Console.hpp"
#include "../domains/Debugger.hpp"
#include "../domains/Fetch.hpp"
#include "../domains/CSS.hpp"
#include "../domains/Storage.hpp"
#include "../domains/Security.hpp"
#include "../domains/Performance.hpp"
#include "../domains/Log.hpp"
#include "../domains/IO.hpp"
#include "../domains/Profiler.hpp"
#include "../domains/HeapProfiler.hpp"
#include "../domains/Accessibility.hpp"
#include "../domains/Memory.hpp"
#include "../domains/Tracing.hpp"
#include "../domains/DOMSnapshot.hpp"
#include "../domains/DOMDebugger.hpp"
#include "../domains/LayerTree.hpp"
#include "../domains/ServiceWorker.hpp"
#include "../domains/IndexedDB.hpp"
#include "../domains/CacheStorage.hpp"
#include "../domains/Overlay.hpp"
#include "../domains/SystemInfo.hpp"
#include "../domains/HeadlessExperimental.hpp"
#include "../domains/Media.hpp"
#include "../domains/WebAuthn.hpp"
#include "../domains/Animation.hpp"
#include "../domains/Audits.hpp"
#include "../domains/Autofill.hpp"
#include "../domains/BackgroundService.hpp"
#include "../domains/BluetoothEmulation.hpp"
#include "../domains/Cast.hpp"
#include "../domains/DeviceAccess.hpp"
#include "../domains/DeviceOrientation.hpp"
#include "../domains/DOMStorage.hpp"
#include "../domains/EventBreakpoints.hpp"
#include "../domains/Extensions.hpp"
#include "../domains/FedCm.hpp"
#include "../domains/FileSystem.hpp"
#include "../domains/Inspector.hpp"
#include "../domains/PerformanceTimeline.hpp"
#include "../domains/Preload.hpp"
#include "../domains/PWA.hpp"
#include "../domains/Tethering.hpp"
#include "../domains/WebAudio.hpp"
#include <memory>
#include <functional>
#include <chrono>

namespace cdp {


struct CDPClientConfig {
    std::string host = "localhost";
    int port = 9222;                    
    int connectionTimeoutMs = 30000;    
    int commandTimeoutMs = 30000;       
    bool autoEnableDomains = true;
    bool useBackgroundThread = true;    
    bool verbose = false;

    
    bool enableHeartbeat = true;        
    int heartbeatIntervalMs = 15000;    

    
    bool autoReconnect = true;          
    int reconnectDelayMs = 1000;        
    int reconnectMaxDelayMs = 30000;    
    int reconnectMaxAttempts = 0;       
    double reconnectBackoffMultiplier = 2.0;  

    
    std::string validate() const {
        if (host.empty()) {
            return "Host cannot be empty";
        }
        if (port < 1 || port > 65535) {
            return "Port must be between 1 and 65535";
        }
        if (connectionTimeoutMs <= 0) {
            return "Connection timeout must be positive";
        }
        if (commandTimeoutMs <= 0) {
            return "Command timeout must be positive";
        }
        if (heartbeatIntervalMs < 1000) {
            return "Heartbeat interval must be at least 1000ms";
        }
        if (reconnectDelayMs < 100) {
            return "Reconnect delay must be at least 100ms";
        }
        return "";  
    }

    bool isValid() const {
        return validate().empty();
    }
};


class CDPClient {
public:
    CDPClient();
    explicit CDPClient(const CDPClientConfig& config);
    ~CDPClient();

    
    CDPClient(const CDPClient&) = delete;
    CDPClient& operator=(const CDPClient&) = delete;
    CDPClient(CDPClient&&) = delete;
    CDPClient& operator=(CDPClient&&) = delete;

    
    [[nodiscard]] bool connect(int targetIndex = 0);

    
    [[nodiscard]] bool connect(const std::string& webSocketUrl);

    
    [[nodiscard]] bool connectToTarget(const CDPTarget& target);

    
    [[nodiscard]] bool connectToBrowser();

    
    void disconnect();

    
    [[nodiscard]] bool isConnected() const;

    
    [[nodiscard]] const std::string& lastError() const;

    
    std::vector<CDPTarget> listTargets();

    
    CDPBrowserInfo getBrowserInfo();

    
    Page Page{connection_};
    Runtime Runtime{connection_};
    DOM DOM{connection_};
    Network Network{connection_};
    Input Input{connection_};
    Emulation Emulation{connection_};
    Target Target{connection_};
    Browser Browser{connection_};
    Console Console{connection_};
    Debugger Debugger{connection_};
    Fetch Fetch{connection_};
    CSS CSS{connection_};
    Storage Storage{connection_};
    Security Security{connection_};
    Performance Performance{connection_};
    Log Log{connection_};
    IO IO{connection_};
    Profiler Profiler{connection_};
    HeapProfiler HeapProfiler{connection_};
    Accessibility Accessibility{connection_};
    Memory Memory{connection_};
    Tracing Tracing{connection_};
    DOMSnapshot DOMSnapshot{connection_};
    DOMDebugger DOMDebugger{connection_};
    LayerTree LayerTree{connection_};
    ServiceWorker ServiceWorker{connection_};
    IndexedDB IndexedDB{connection_};
    CacheStorage CacheStorage{connection_};
    Overlay Overlay{connection_};
    SystemInfo SystemInfo{connection_};
    HeadlessExperimental HeadlessExperimental{connection_};
    Media Media{connection_};
    WebAuthn WebAuthn{connection_};
    Animation Animation{connection_};
    Audits Audits{connection_};
    Autofill Autofill{connection_};
    BackgroundService BackgroundService{connection_};
    BluetoothEmulation BluetoothEmulation{connection_};
    Cast Cast{connection_};
    DeviceAccess DeviceAccess{connection_};
    DeviceOrientation DeviceOrientation{connection_};
    DOMStorage DOMStorage{connection_};
    EventBreakpoints EventBreakpoints{connection_};
    Extensions Extensions{connection_};
    FedCm FedCm{connection_};
    FileSystem FileSystem{connection_};
    Inspector Inspector{connection_};
    PerformanceTimeline PerformanceTimeline{connection_};
    Preload Preload{connection_};
    PWA PWA{connection_};
    Tethering Tethering{connection_};
    WebAudio WebAudio{connection_};

    
    CDPConnection& connection() { return connection_; }

    
    CDPResponse sendCommand(const std::string& method, const JsonValue& params = JsonValue());

    
    void poll(int timeoutMs = 100);

    
    void startBackgroundThread() { connection_.startMessageThread(); }
    void stopBackgroundThread() { connection_.stopMessageThread(); }
    bool isBackgroundThreadRunning() const { return connection_.isMessageThreadRunning(); }

    
    bool waitFor(std::function<bool()> condition, int timeoutMs = 30000, int pollIntervalMs = 100);

    
    static void sleep(int ms);

    
    const CDPClientConfig& config() const { return config_; }

private:
    void enableDomains();
    void setError(const std::string& error) { lastError_ = error; }

    CDPClientConfig config_;
    CDPConnection connection_;
    std::atomic<bool> pageLoaded_{false};
    std::string lastError_;  
};

} 
