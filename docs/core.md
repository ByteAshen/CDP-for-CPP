# Core Components Reference

Low-level utilities and helpers used throughout the library.

## Json

JSON parsing and serialization.

```cpp
#include <cdp/core/Json.hpp>
using namespace cdp;
```

### JsonValue

Represents any JSON value (null, bool, number, string, array, object).

```cpp
// Constructors
JsonValue v1;                      // null
JsonValue v2(true);                // bool
JsonValue v3(42);                  // int
JsonValue v4(3.14);                // double
JsonValue v5("hello");             // string
JsonValue v6(JsonArray{});         // array
JsonValue v7(JsonObject{});        // object

// Type checking
v1.isNull();    // true
v2.isBool();    // true
v3.isInt();     // true
v4.isDouble();  // true
v5.isString();  // true
v6.isArray();   // true
v7.isObject();  // true

// Value access with defaults
bool b = v2.getBool(false);
int i = v3.getInt(0);
int64_t i64 = v3.getInt64(0);
double d = v4.getNumber(0.0);
std::string s = v5.getString("");
uint64_t u = v3.getUint64(0);

// Throwing accessors
bool b = v2.asBool();
int64_t i = v3.asInt64();
double d = v4.asDouble();
const std::string& s = v5.asString();
const JsonArray& arr = v6.asArray();
const JsonObject& obj = v7.asObject();

// Optional accessors
std::optional<bool> b = v2.tryBool();
std::optional<int64_t> i = v3.tryInt64();
std::optional<double> d = v4.tryDouble();
std::optional<std::string> s = v5.tryString();
```

### Object Access

```cpp
JsonObject obj;
obj["name"] = "John";
obj["age"] = 30;
obj["active"] = true;

// Access
std::string name = obj["name"].getString();
int age = obj["age"].getInt();

// Check existence
if (obj.contains("email")) { /* ... */ }

// Find (returns nullptr if not found)
const JsonValue* val = obj.find("name");

// Safe access with at()
const JsonValue& val = obj.at("name");  // throws if not found
```

### Array Access

```cpp
JsonArray arr;
arr.push_back(1);
arr.push_back("two");
arr.push_back(3.0);

// Access by index
int first = arr[0].getInt();
std::string second = arr[1].getString();

// Size
size_t count = arr.size();
bool empty = arr.empty();

// Iteration
for (const auto& item : arr) {
    // ...
}
```

### Path Access

Navigate nested structures:

```cpp
JsonValue data = JsonValue::parse(R"({
    "user": {
        "name": "John",
        "address": {
            "city": "NYC"
        },
        "tags": ["admin", "user"]
    }
})");

// Dot notation
std::string city = data.getPath("user.address.city").getString();

// Array index
std::string tag = data.getPath("user.tags.0").getString();

// At-path helpers
int count = data.getIntAt("user.posts.count", 0);
std::string name = data.getStringAt("user.name", "Unknown");
bool active = data.getBoolAt("user.active", false);

// Check path exists
if (data.hasPath("user.email")) { /* ... */ }
```

### Parsing & Serialization

```cpp
// Parse JSON string
JsonValue data = JsonValue::parse(R"({"key": "value"})");

// Serialize to string
std::string json = data.serialize();

// Pretty print
std::string pretty = data.serialize(true);
```

### Params Builder

Build CDP command parameters:

```cpp
auto params = Params()
    .set("url", "https://example.com")
    .set("timeout", 30000)
    .set("waitUntil", "load")
    .build();

client.sendCommand("Page.navigate", params);
```

---

## Base64

Base64 encoding/decoding.

```cpp
#include <cdp/core/Base64.hpp>
using namespace cdp;

// Encode string
std::string encoded = Base64::encode("Hello World");

// Encode binary data
std::vector<uint8_t> data = {0x48, 0x65, 0x6c, 0x6c, 0x6f};
std::string encoded = Base64::encode(data);
std::string encoded = Base64::encode(data.data(), data.size());

// Decode to string
std::string decoded = Base64::decodeToString(encoded);

// Decode to binary
std::vector<uint8_t> binary = Base64::decode(encoded);
```

---

## SHA1

SHA1 hashing (used for WebSocket handshake).

```cpp
#include <cdp/core/SHA1.hpp>
using namespace cdp;

// Hash string, get hex result
std::string hex = SHA1::hashHex("Hello World");

// Hash string, get binary result
std::vector<uint8_t> binary = SHA1::hash("Hello World");

// Incremental hashing
SHA1 hasher;
hasher.update("Hello ");
hasher.update("World");
std::string hex = hasher.finalizeHex();
std::vector<uint8_t> binary = hasher.finalize();
```

---

## Logger

Configurable logging.

```cpp
#include <cdp/core/Logger.hpp>
using namespace cdp;

// Log levels
Logger::setLevel(LogLevel::Debug);   // All messages
Logger::setLevel(LogLevel::Info);    // Info and above
Logger::setLevel(LogLevel::Warning); // Warnings and errors
Logger::setLevel(LogLevel::Error);   // Errors only
Logger::setLevel(LogLevel::None);    // No logging

// Custom output
Logger::setOutput([](LogLevel level, const std::string& msg) {
    std::cout << "[" << levelName(level) << "] " << msg << std::endl;
});

// Log messages
Logger::debug("Debug message");
Logger::info("Info message");
Logger::warning("Warning message");
Logger::error("Error message");
```

---

## Enums

String conversion for CDP enumerations.

```cpp
#include <cdp/core/Enums.hpp>
using namespace cdp;

// Transition types
std::string str = transitionTypeToString(TransitionType::Link);
TransitionType type = transitionTypeFromString("typed");

// Mouse buttons
std::string str = mouseButtonToString(MouseButton::Left);
MouseButton btn = mouseButtonFromString("right");

// Resource types
std::string str = resourceTypeToString(ResourceType::Document);
ResourceType res = resourceTypeFromString("script");

// Frame lifecycle
std::string str = frameLifecycleToString(FrameLifecycle::Load);

// Dialog types
std::string str = dialogTypeToString(DialogType::Alert);
```

---

## TypedResponses

Typed wrappers for CDP responses.

```cpp
#include <cdp/core/TypedResponses.hpp>
using namespace cdp;

// Example: Get document response
auto response = client.DOM.getDocument();
GetDocumentResponse doc(response);

int rootNodeId = doc.rootNodeId();
std::string nodeName = doc.nodeName();
std::vector<int> childNodeIds = doc.childNodeIds();

// Example: Evaluate response
auto response = client.Runtime.evaluate("1 + 1");
EvaluateResponse eval(response);

std::string type = eval.type();           // "number"
JsonValue value = eval.value();           // 2
bool wasThrown = eval.exceptionThrown();
std::string objectId = eval.objectId();   // For complex objects
```

---

## CDPTarget

Information about a browser target (tab, worker, etc.).

```cpp
struct CDPTarget {
    std::string description;
    std::string devtoolsFrontendUrl;
    std::string faviconUrl;
    std::string id;
    std::string parentId;
    std::string title;
    std::string type;  // "page", "worker", "service_worker", etc.
    std::string url;
    std::string webSocketDebuggerUrl;

    static CDPTarget fromJson(const JsonValue& json);
};
```

Usage:

```cpp
auto targets = client.listTargets();
for (const auto& target : targets) {
    if (target.type == "page") {
        std::cout << target.title << " - " << target.url << std::endl;
    }
}
```

---

## CDPBrowserInfo

Browser information.

```cpp
struct CDPBrowserInfo {
    std::string browser;              // "Chrome/120.0.0.0"
    std::string protocolVersion;      // "1.3"
    std::string userAgent;
    std::string v8Version;
    std::string webKitVersion;
    std::string webSocketDebuggerUrl;

    static CDPBrowserInfo fromJson(const JsonValue& json);
};
```

Usage:

```cpp
auto info = client.getBrowserInfo();
std::cout << "Browser: " << info.browser << std::endl;
std::cout << "Protocol: " << info.protocolVersion << std::endl;
```

---

## CDPResponse

Response from a CDP command.

```cpp
struct CDPResponse {
    int id;
    bool hasError;
    int errorCode;
    std::string errorMessage;
    JsonValue result;

    CDPErrorCategory errorCategory() const;
    bool hasException() const;
    std::string exceptionText() const;
};
```

Usage:

```cpp
auto response = client.sendCommand("Page.navigate",
    Params().set("url", "https://example.com").build());

if (response.hasError) {
    std::cerr << "Error: " << response.errorMessage << std::endl;
    std::cerr << "Category: " << static_cast<int>(response.errorCategory()) << std::endl;
} else {
    std::string frameId = response.result["frameId"].getString();
}
```

---

## CDPEvent

Event received from CDP.

```cpp
struct CDPEvent {
    std::string method;   // e.g., "Page.loadEventFired"
    JsonValue params;     // Event parameters
};
```

---

## CDPClientConfig

Client configuration.

```cpp
struct CDPClientConfig {
    std::string host = "localhost";
    int port = 9222;
    int connectionTimeoutMs = 30000;
    int commandTimeoutMs = 30000;
    bool autoEnableDomains = true;
    bool useBackgroundThread = true;
    bool verbose = false;

    // Heartbeat
    bool enableHeartbeat = true;
    int heartbeatIntervalMs = 15000;

    // Reconnect
    bool autoReconnect = true;
    int reconnectDelayMs = 1000;
    int reconnectMaxDelayMs = 30000;
    int reconnectMaxAttempts = 0;
    double reconnectBackoffMultiplier = 2.0;

    std::string validate() const;  // Returns error message or empty
    bool isValid() const;
};
```

---

## Network Types

### HeaderEntry

HTTP header manipulation.

```cpp
struct HeaderEntry {
    std::string name;
    std::string value;

    // Extract from request
    static std::vector<HeaderEntry> fromRequest(const JsonObject& request);

    // Convert to JSON
    static JsonArray toJson(const std::vector<HeaderEntry>& headers);

    // Modify headers
    static void set(std::vector<HeaderEntry>& headers, const std::string& name, const std::string& value);
    static void remove(std::vector<HeaderEntry>& headers, const std::string& name);
    static std::string get(const std::vector<HeaderEntry>& headers, const std::string& name);
};
```

### RequestPattern

For fetch interception.

```cpp
struct RequestPattern {
    std::string urlPattern = "*";
    std::string resourceType;  // "Document", "Script", "Image", etc.
    std::string requestStage = "Request";

    static RequestPattern all();
    static RequestPattern documents();
    static RequestPattern scripts();
    static RequestPattern images();
    static RequestPattern xhr();
    static RequestPattern url(const std::string& pattern);
};
```

---

## Utility Functions

### Stealth Helpers

Human-like behavior simulation.

```cpp
#include <cdp/highlevel/Utilities.hpp>
using namespace cdp;

// Random delay (for human-like behavior)
int delay = humanDelay(100, 300);  // Random between 100-300ms

// Human-like mouse movement path
auto path = humanMousePath(0, 0, 100, 200, 10);  // 10 intermediate points
for (const auto& [x, y] : path) {
    client.Input.dispatchMouseEvent("mouseMoved", x, y);
    std::this_thread::sleep_for(std::chrono::milliseconds(humanDelay(5, 15)));
}
```

### Cookie Data

```cpp
struct CookieData {
    std::string name;
    std::string value;
    std::string domain;
    std::string path;
    double expires;
    bool secure;
    bool httpOnly;
    std::string sameSite;
    std::string priority;

    bool isExpired() const;
    bool matchesDomain(const std::string& domain) const;
    static CookieData fromJson(const JsonValue& json);
    JsonValue toJson() const;
};
```

### Performance Metrics

```cpp
struct PerformanceMetrics {
    double navigationStart;
    double domContentLoaded;
    double load;
    double firstPaint;
    double firstContentfulPaint;
    double largestContentfulPaint;
    double timeToInteractive;
    int64_t jsHeapUsedSize;
    int64_t jsHeapTotalSize;

    JsonValue toJson() const;
};
```
