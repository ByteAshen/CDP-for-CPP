# Browser & Context API Reference

Manage multiple pages and browser contexts.

## Browser Class

The `Browser` class manages browser connections and creates pages/contexts.

```cpp
#include <cdp/highlevel/Browser.hpp>
using namespace cdp::highlevel;
```

### Constructor

```cpp
Browser();
Browser(const CDPClientConfig& config);
```

### Connection

```cpp
Result<void> connect();        // Connect to browser
void disconnect();             // Disconnect
bool isConnected() const;      // Check connection
```

**Example:**
```cpp
Browser browser;
auto result = browser.connect();
if (!result) {
    std::cerr << "Failed: " << result.error().message << std::endl;
}
```

### Browser Info

```cpp
std::string version();         // Browser version string
std::string userAgent();       // Default user agent
```

### Page Management

```cpp
Result<ManagedPage*> newPage(const NewPageOptions& options = {});
std::vector<ManagedPage*> pages();
```

**NewPageOptions:**
```cpp
struct NewPageOptions {
    std::string url = "about:blank";  // Initial URL
    int width = 0;                     // Viewport width (0 = default)
    int height = 0;                    // Viewport height (0 = default)
    bool background = false;           // Create in background
};
```

**Example:**
```cpp
// Simple
auto page = browser.newPage();
page->page().navigate("https://example.com");

// With options
NewPageOptions opts;
opts.url = "https://example.com";
opts.width = 1920;
opts.height = 1080;
auto page = browser.newPage(opts);
```

### Context Management

```cpp
BrowserContext& defaultContext();
Result<BrowserContext*> createIncognitoContext(const BrowserContextOptions& options = {});
std::vector<BrowserContext*> contexts();
```

**BrowserContextOptions:**
```cpp
struct BrowserContextOptions {
    bool disposeOnDetach = true;
    std::string proxyServer;        // e.g., "http://proxy:8080"
    std::string proxyBypassList;    // Comma-separated hosts
    std::optional<ProxyCredentials> proxyCredentials;
};

struct ProxyCredentials {
    std::string username;
    std::string password;
};
```

**Example:**
```cpp
// Create incognito context
auto context = browser.createIncognitoContext();
auto page = context->newPage();

// With proxy
BrowserContextOptions opts;
opts.proxyServer = "http://proxy.example.com:8080";
opts.proxyCredentials = ProxyCredentials{"user", "pass"};
auto proxyContext = browser.createIncognitoContext(opts);
```

### Lifecycle

```cpp
Result<void> close();          // Close browser
```

---

## BrowserContext Class

Isolated browser context (like incognito mode).

### Methods

```cpp
const std::string& id() const;              // Context ID
bool isDefault() const;                      // Is default context?
Result<ManagedPage*> newPage(const NewPageOptions& options = {});
std::vector<ManagedPage*> pages();
Result<void> close();                        // Close context and all pages
Browser& browser();                          // Parent browser
```

**Example:**
```cpp
auto context = browser.createIncognitoContext();

// Create multiple pages in context
auto page1 = context->newPage("https://example.com");
auto page2 = context->newPage("https://google.com");

// Each context has isolated cookies/storage
page1->page().setCookie("session", "abc", "example.com");
// page2 won't see this cookie

// Close context (closes all pages)
context->close();
```

---

## ManagedPage Class

A page with its own CDP client connection.

### Methods

```cpp
Page& page();                  // Get Page interface
CDPClient& client();           // Get CDP client
const std::string& targetId() const;
const std::string& contextId() const;
bool isConnected() const;
Result<void> close();
Result<void> bringToFront();
```

**Example:**
```cpp
auto managed = browser.newPage();

// Use high-level Page API
managed->page().navigate("https://example.com");
managed->page().click("#button");

// Or low-level CDP client
managed->client().Runtime.evaluate("console.log('hello')");

// Close
managed->close();
```

---

## ScopedPage

RAII wrapper for automatic page cleanup.

```cpp
{
    ScopedPage page(browser, "https://example.com");
    if (page) {
        page->page().click("#button");
    }
}  // Page automatically closed here
```

### Constructor

```cpp
ScopedPage(Browser& browser, const std::string& url = "about:blank");
ScopedPage(BrowserContext& context, const std::string& url = "about:blank");
```

### Methods

```cpp
bool valid() const;
explicit operator bool() const;
ManagedPage* get();
ManagedPage* operator->();
Page* page();
CDPClient* client();
void close();              // Close early
ManagedPage* release();    // Release ownership
```

---

## ScopedContext

RAII wrapper for automatic context cleanup.

```cpp
{
    ScopedContext context(browser);
    if (context) {
        auto page = context->newPage("https://example.com");
    }
}  // Context and all pages closed
```

### Methods

```cpp
bool valid() const;
explicit operator bool() const;
BrowserContext* get();
BrowserContext* operator->();
void close();
BrowserContext* release();
```

---

## Complete Example

```cpp
#include <cdp/highlevel/Browser.hpp>
#include <iostream>

using namespace cdp::highlevel;

int main() {
    Browser browser;

    if (!browser.connect()) {
        std::cerr << "Failed to connect" << std::endl;
        return 1;
    }

    std::cout << "Browser: " << browser.version() << std::endl;

    // Create page in default context
    auto defaultPage = browser.newPage();
    defaultPage->page().navigate("https://example.com");

    // Create incognito context
    auto context = browser.createIncognitoContext();
    if (context) {
        auto privatePage = context.value()->newPage();
        privatePage->page().navigate("https://private.example.com");

        // Different cookies than defaultPage
        privatePage->page().setCookie("private", "data", "private.example.com");
    }

    // Using ScopedPage for automatic cleanup
    {
        ScopedPage tempPage(browser, "https://temp.example.com");
        if (tempPage) {
            tempPage->page().screenshot("temp.png");
        }
    }  // tempPage closed automatically

    // Close browser
    browser.close();

    return 0;
}
```
