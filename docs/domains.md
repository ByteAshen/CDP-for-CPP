# CDP Domains Reference

Low-level access to Chrome DevTools Protocol domains.

## CDPClient

The `CDPClient` class provides direct access to all CDP domains.

```cpp
#include <cdp/protocol/CDPClient.hpp>
using namespace cdp;
```

### Configuration

```cpp
CDPClientConfig config;
config.host = "localhost";           // Browser host
config.port = 9222;                  // Debug port
config.connectionTimeoutMs = 30000;  // Connection timeout
config.commandTimeoutMs = 30000;     // Command timeout
config.autoEnableDomains = true;     // Auto-enable common domains
config.useBackgroundThread = true;   // Background message processing
config.verbose = false;              // Debug logging

// Heartbeat
config.enableHeartbeat = true;
config.heartbeatIntervalMs = 15000;

// Auto-reconnect
config.autoReconnect = true;
config.reconnectDelayMs = 1000;
config.reconnectMaxDelayMs = 30000;
config.reconnectMaxAttempts = 0;     // 0 = unlimited

CDPClient client(config);
```

### Connection

```cpp
// Connect to target by index
client.connect(0);  // First tab

// Connect by WebSocket URL
client.connect("ws://localhost:9222/devtools/page/ABC123");

// Connect to browser endpoint
client.connectToBrowser();

// Disconnect
client.disconnect();

// Check connection
if (client.isConnected()) { /* ... */ }

// Get error message
std::string err = client.lastError();
```

### Target Discovery

```cpp
// List available targets
std::vector<CDPTarget> targets = client.listTargets();
for (const auto& target : targets) {
    std::cout << target.title << " - " << target.url << std::endl;
}

// Get browser info
CDPBrowserInfo info = client.getBrowserInfo();
std::cout << info.browser << std::endl;
```

---

## Domain Access

All CDP domains are accessible as members of CDPClient:

```cpp
client.Page.navigate("https://example.com");
client.DOM.getDocument();
client.Runtime.evaluate("1 + 1");
client.Network.setCookie("name", "value", "example.com");
```

### Available Domains

| Domain | Description |
|--------|-------------|
| **Page** | Page lifecycle, navigation, screenshots, PDF |
| **Runtime** | JavaScript execution and object inspection |
| **DOM** | DOM tree inspection and manipulation |
| **Network** | Network monitoring, cookies, request blocking |
| **Input** | Mouse, keyboard, touch simulation |
| **Emulation** | Device emulation, geolocation, sensors |
| **Target** | Target (tab/page) management |
| **Browser** | Browser-level operations |
| **Console** | Console message handling |
| **Debugger** | JavaScript debugging |
| **Fetch** | Request interception and modification |
| **CSS** | CSS inspection and modification |
| **Storage** | Storage quota and clearing |
| **Security** | Security state and certificates |
| **Performance** | Performance metrics |
| **Log** | Browser log messages |
| **IO** | Stream reading |
| **Profiler** | JavaScript profiling |
| **HeapProfiler** | Heap snapshots |
| **Accessibility** | Accessibility tree |
| **Memory** | Memory pressure |
| **Tracing** | Performance tracing |
| **DOMSnapshot** | DOM snapshots |
| **DOMDebugger** | DOM breakpoints |
| **LayerTree** | Compositing layers |
| **ServiceWorker** | Service worker inspection |
| **IndexedDB** | IndexedDB inspection |
| **CacheStorage** | Cache API inspection |
| **Overlay** | DOM highlighting |
| **SystemInfo** | System information |
| **HeadlessExperimental** | Headless-specific features |
| **Media** | Media playback |
| **WebAuthn** | WebAuthn emulation |
| **Animation** | Animation inspection |
| **Audits** | Deprecation and issues |
| **Autofill** | Form autofill |
| **BackgroundService** | Background services |
| **BluetoothEmulation** | Bluetooth emulation |
| **Cast** | Cast discovery |
| **DeviceAccess** | Device access |
| **DeviceOrientation** | Orientation emulation |
| **DOMStorage** | Local/session storage |
| **EventBreakpoints** | Event breakpoints |
| **Extensions** | Extension inspection |
| **FedCm** | Federated credentials |
| **FileSystem** | File system access |
| **Inspector** | Inspector state |
| **PerformanceTimeline** | Performance timeline |
| **Preload** | Preload inspection |
| **PWA** | Progressive Web App |
| **Tethering** | Port forwarding |
| **WebAudio** | Web Audio API |

---

## Common Domain Operations

### Page Domain

```cpp
// Enable domain (required before use)
client.Page.enable();

// Navigation
client.Page.navigate("https://example.com");
client.Page.reload();
client.Page.stopLoading();

// History
auto history = client.Page.getNavigationHistory();
client.Page.navigateToHistoryEntry(entryId);

// Screenshots
auto response = client.Page.captureScreenshot("png", 80, false);
std::string base64Data = response.result["data"].getString();

// PDF (headless only)
auto pdf = client.Page.printToPDF();

// Lifecycle
client.Page.close();
client.Page.bringToFront();

// Events
client.Page.onLoadEventFired([]() {
    std::cout << "Page loaded!" << std::endl;
});

client.Page.onFrameNavigated([](const std::string& frameId) {
    std::cout << "Frame navigated: " << frameId << std::endl;
});
```

### Runtime Domain

```cpp
client.Runtime.enable();

// Evaluate JavaScript
auto result = client.Runtime.evaluate("document.title");
std::string title = result.result["result"]["value"].getString();

// With await
auto asyncResult = client.Runtime.evaluate("await fetch('/api/data').then(r => r.json())");

// Call function
auto callResult = client.Runtime.callFunctionOn(objectId, "function() { return this.value; }");

// Get properties
auto props = client.Runtime.getProperties(objectId);

// Console events
client.Runtime.onConsoleAPICalled([](const std::string& type, const std::vector<JsonValue>& args) {
    std::cout << "[" << type << "] " << args[0].getString() << std::endl;
});
```

### DOM Domain

```cpp
client.DOM.enable();

// Get document
auto doc = client.DOM.getDocument();
int rootNodeId = doc.result["root"]["nodeId"].getInt();

// Query selectors
auto node = client.DOM.querySelector(rootNodeId, "#element");
int nodeId = node.result["nodeId"].getInt();

auto nodes = client.DOM.querySelectorAll(rootNodeId, ".items");

// Get attributes
auto attrs = client.DOM.getAttributes(nodeId);

// Get outer HTML
auto html = client.DOM.getOuterHTML(nodeId);

// Set attribute
client.DOM.setAttributeValue(nodeId, "class", "new-class");

// Modify content
client.DOM.setOuterHTML(nodeId, "<div>New content</div>");

// Focus
client.DOM.focus(nodeId);

// Scroll into view
client.DOM.scrollIntoViewIfNeeded(nodeId);

// Box model
auto box = client.DOM.getBoxModel(nodeId);
```

### Network Domain

```cpp
client.Network.enable();

// Cookies
client.Network.setCookie("name", "value", "example.com");
auto cookies = client.Network.getCookies();
client.Network.deleteCookies("name", "example.com");
client.Network.clearBrowserCookies();

// Cache
client.Network.clearBrowserCache();
client.Network.setCacheDisabled(true);

// Headers
client.Network.setExtraHTTPHeaders({
    {"Authorization", "Bearer token"},
    {"X-Custom", "value"}
});

// Block URLs
client.Network.setBlockedURLs({"*.png", "*.jpg", "ads.example.com/*"});

// User agent
client.Network.setUserAgentOverride("Custom Agent");

// Throttling
NetworkConditions conditions;
conditions.offline = false;
conditions.latency = 100;
conditions.downloadThroughput = 1000000;  // 1 MB/s
conditions.uploadThroughput = 500000;      // 500 KB/s
client.Network.emulateNetworkConditions(conditions);

// Events
client.Network.onRequestWillBeSent([](const std::string& requestId, const std::string& url) {
    std::cout << "Request: " << url << std::endl;
});

client.Network.onResponseReceived([](const std::string& requestId, const JsonValue& response) {
    int status = response["status"].getInt();
    std::cout << "Response: " << status << std::endl;
});
```

### Input Domain

```cpp
// Mouse events
client.Input.dispatchMouseEvent("mousePressed", 100, 200, "left", 1);
client.Input.dispatchMouseEvent("mouseReleased", 100, 200, "left", 1);
client.Input.dispatchMouseEvent("mouseMoved", 150, 250);

// Keyboard events
client.Input.dispatchKeyEvent("keyDown", "Enter");
client.Input.dispatchKeyEvent("keyUp", "Enter");

// Type text (fast)
client.Input.insertText("Hello World");

// Touch events
std::vector<TouchPoint> touches = {{100, 200}};
client.Input.dispatchTouchEvent("touchStart", touches);
client.Input.dispatchTouchEvent("touchEnd", touches);

// Synthesize events
client.Input.synthesizePinchGesture(100, 200, 2.0);  // Zoom in 2x
client.Input.synthesizeScrollGesture(100, 200, 0, -500);  // Scroll down
client.Input.synthesizeTapGesture(100, 200);
```

### Emulation Domain

```cpp
// Viewport
client.Emulation.setDeviceMetricsOverride(1920, 1080, 1.0, false);

// Mobile emulation
client.Emulation.setDeviceMetricsOverride(375, 812, 3.0, true);
client.Emulation.setTouchEmulationEnabled(true);

// User agent
client.Emulation.setUserAgentOverride("Mobile Safari...");

// Geolocation
client.Emulation.setGeolocationOverride(37.7749, -122.4194, 100);

// Timezone
client.Emulation.setTimezoneOverride("America/New_York");

// Locale
client.Emulation.setLocaleOverride("fr-FR");

// Media features
client.Emulation.setEmulatedMedia("print", {{"prefers-color-scheme", "dark"}});

// Sensors
client.Emulation.setIdleOverride(true, false);  // User idle, screen unlocked

// Vision deficiency
client.Emulation.setEmulatedVisionDeficiency("deuteranopia");

// CPU throttling
client.Emulation.setCPUThrottlingRate(4.0);  // 4x slower
```

### Fetch Domain

```cpp
// Enable with patterns
std::vector<RequestPattern> patterns = {RequestPattern::all()};
client.Fetch.enable(patterns, true);  // handleAuthRequests = true

// Handle paused requests
client.Fetch.onRequestPaused([&client](const std::string& requestId,
                                        const std::string& url,
                                        const std::string& method,
                                        const JsonValue& headers) {
    // Continue normally
    client.Fetch.continueRequest(requestId);

    // Or modify
    // client.Fetch.continueRequest(requestId, url, method, postData, modifiedHeaders);

    // Or block
    // client.Fetch.failRequest(requestId, "BlockedByClient");

    // Or fulfill with custom response
    // client.Fetch.fulfillRequest(requestId, 200, responseHeaders, body);
});

// Handle auth challenges
client.Fetch.onAuthRequired([&client](const std::string& requestId, const std::string& authChallenge) {
    client.Fetch.continueWithAuth(requestId, "ProvideCredentials", "username", "password");
});

client.Fetch.disable();
```

### Debugger Domain

```cpp
client.Debugger.enable();

// Set breakpoint
auto bp = client.Debugger.setBreakpointByUrl(10, "script.js");
std::string breakpointId = bp.result["breakpointId"].getString();

// Set breakpoint on function
client.Debugger.setBreakpointOnFunctionCall(objectId);

// Pause/Resume
client.Debugger.pause();
client.Debugger.resume();
client.Debugger.stepOver();
client.Debugger.stepInto();
client.Debugger.stepOut();

// Remove breakpoint
client.Debugger.removeBreakpoint(breakpointId);

// Events
client.Debugger.onPaused([](const std::vector<CallFrame>& callFrames, const std::string& reason) {
    std::cout << "Paused at: " << callFrames[0].functionName << std::endl;
});

client.Debugger.onScriptParsed([](const std::string& scriptId, const std::string& url) {
    std::cout << "Script loaded: " << url << std::endl;
});
```

---

## Raw Commands

Send any CDP command:

```cpp
auto response = client.sendCommand("Page.navigate",
    Params()
        .set("url", "https://example.com")
        .set("transitionType", "typed")
        .build());

if (!response.hasError) {
    std::string frameId = response.result["frameId"].getString();
}
```

---

## Event Handling

```cpp
// One-time handler
client.Page.onLoadEventFired([&]() {
    // Runs once, then removed
});

// Persistent handler
auto token = client.Page.onLoadEventFired([&]() {
    // Runs every time
});

// Remove handler
client.Page.removeListener(token);
```

---

## Thread Safety

- All domain method calls are thread-safe
- Multiple threads can call methods concurrently
- Event handlers run on background thread (if enabled)
- Don't call sync methods from event handlers

```cpp
// Safe: Async calls in handlers
client.Page.onLoadEventFired([&client]() {
    client.Page.captureScreenshotAsync();
});

// UNSAFE: Sync calls in handlers can deadlock
// client.Page.onLoadEventFired([&client]() {
//     client.Page.captureScreenshot();  // DON'T DO THIS
// });
```

---

## Domain Dependencies

Some domains require others to be enabled first:

| Domain | Requires |
|--------|----------|
| CSS | DOM |
| DOMSnapshot | DOM |
| Overlay | DOM |
| DOMDebugger | DOM |
| Accessibility (some methods) | DOM |
| LayerTree (some methods) | DOM |

```cpp
// Correct order
client.DOM.enable();
client.CSS.enable();

// Or auto-enable handles this
CDPClientConfig config;
config.autoEnableDomains = true;
CDPClient client(config);
```
