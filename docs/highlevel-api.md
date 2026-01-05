# High-Level API Reference

The high-level API provides production-ready browser automation with proper error handling, retry logic, and resource management.

## Namespace

```cpp
#include <cdp/highlevel/Page.hpp>
#include <cdp/highlevel/Browser.hpp>
#include <cdp/highlevel/ElementHandle.hpp>
#include <cdp/highlevel/Result.hpp>

using namespace cdp::highlevel;
```

---

## Page Class

The `Page` class provides Puppeteer/Playwright-style operations.

### Constructor

```cpp
Page(CDPClient& client);
```

Creates a Page wrapper around an existing CDP client connection.

### Navigation Methods

#### navigate

```cpp
Result<void> navigate(const std::string& url, int timeoutMs = 30000);
Result<void> navigate(const std::string& url, const NavigateOptions& options);
```

Navigate to a URL and wait for the page to load.

**Parameters:**
- `url` - The URL to navigate to
- `timeoutMs` - Maximum time to wait (default: 30000ms)
- `options` - Navigation options (see NavigateOptions)

**Example:**
```cpp
auto result = page.navigate("https://example.com");
if (!result) {
    std::cerr << "Navigation failed: " << result.error().message << std::endl;
}

// With options
NavigateOptions opts;
opts.waitUntil = WaitUntil::NetworkIdle0;
opts.timeoutMs = 60000;
page.navigate("https://example.com", opts);
```

#### navigateNoWait

```cpp
Result<void> navigateNoWait(const std::string& url);
```

Start navigation without waiting for completion.

#### reload

```cpp
Result<void> reload(int timeoutMs = 30000);
Result<void> reload(const NavigateOptions& options);
```

Reload the current page.

#### goBack / goForward

```cpp
Result<void> goBack(int timeoutMs = 30000);
Result<void> goForward(int timeoutMs = 30000);
```

Navigate browser history.

#### url / title

```cpp
std::string url();
std::string title();
```

Get current URL or page title.

---

### Element Interaction

#### click

```cpp
Result<void> click(const std::string& selector, int timeoutMs = 30000);
```

Click an element. Waits for element to exist, scrolls into view, then clicks.

**Example:**
```cpp
page.click("#submit-button");
page.click("button[type='submit']");
page.click(".nav-link:first-child");
```

#### type

```cpp
Result<void> type(const std::string& selector, const std::string& text, int timeoutMs = 30000);
Result<void> type(const std::string& selector, const std::string& text, const TypeOptions& options, int timeoutMs = 30000);
```

Type text into an input element. By default, clears existing content first.

**Example:**
```cpp
page.type("#email", "user@example.com");

// Without clearing
TypeOptions opts;
opts.clearFirst = false;
page.type("#search", "additional text", opts);

// Human-like typing with delays
TypeOptions humanOpts;
humanOpts.useKeyEvents = true;
humanOpts.delayMs = 50;
page.type("#password", "secret", humanOpts);
```

#### typeAppend

```cpp
Result<void> typeAppend(const std::string& selector, const std::string& text, int timeoutMs = 30000);
```

Type text without clearing existing content.

#### clear

```cpp
Result<void> clear(const std::string& selector, int timeoutMs = 30000);
```

Clear an input field.

#### focus

```cpp
Result<void> focus(const std::string& selector, int timeoutMs = 30000);
```

Focus an element.

#### hover

```cpp
Result<void> hover(const std::string& selector, int timeoutMs = 30000);
```

Move mouse over an element.

#### check

```cpp
Result<void> check(const std::string& selector, bool checked = true, int timeoutMs = 30000);
```

Check or uncheck a checkbox/radio button.

#### select

```cpp
Result<void> select(const std::string& selector, const std::string& value, int timeoutMs = 30000);
```

Select an option in a dropdown by value.

#### press

```cpp
Result<void> press(const std::string& key, int timeoutMs = 30000);
```

Press a keyboard key.

**Example:**
```cpp
page.press("Enter");
page.press("Tab");
page.press("Escape");
page.press("ArrowDown");
```

#### doubleClick

```cpp
Result<void> doubleClick(const std::string& selector, int timeoutMs = 30000);
```

Double-click an element.

#### rightClick

```cpp
Result<void> rightClick(const std::string& selector, int timeoutMs = 30000);
```

Right-click an element (context menu).

#### dragAndDrop

```cpp
Result<void> dragAndDrop(const std::string& sourceSelector, const std::string& targetSelector, int timeoutMs = 30000);
```

Drag an element to another element.

#### uploadFile

```cpp
Result<void> uploadFile(const std::string& selector, const std::vector<std::string>& filePaths, int timeoutMs = 30000);
```

Upload files to a file input.

**Example:**
```cpp
page.uploadFile("#file-input", {"document.pdf", "image.png"});
```

---

### Content Retrieval

#### getText

```cpp
Result<std::string> getText(const std::string& selector, int timeoutMs = 30000);
```

Get text content of an element.

#### getHTML

```cpp
Result<std::string> getHTML(const std::string& selector, int timeoutMs = 30000);
```

Get inner HTML of an element.

#### getAttribute

```cpp
Result<std::string> getAttribute(const std::string& selector, const std::string& name, int timeoutMs = 30000);
```

Get attribute value.

#### getValue

```cpp
Result<std::string> getValue(const std::string& selector, int timeoutMs = 30000);
```

Get input element value.

#### setValue

```cpp
Result<void> setValue(const std::string& selector, const std::string& value, int timeoutMs = 30000);
```

Set input element value directly (without typing).

#### content

```cpp
Result<std::string> content();
```

Get entire page HTML.

#### setContent

```cpp
Result<void> setContent(const std::string& html);
```

Replace page content with HTML.

---

### Element Queries

#### querySelector

```cpp
Result<ElementHandle> querySelector(const std::string& selector);
```

Find first matching element.

#### querySelectorAll

```cpp
Result<std::vector<ElementHandle>> querySelectorAll(const std::string& selector);
```

Find all matching elements.

#### exists

```cpp
bool exists(const std::string& selector);
```

Check if element exists (no waiting).

#### count

```cpp
int count(const std::string& selector);
```

Count matching elements.

---

### Waiting Methods

All wait methods support `WaitOptions` for customization.

#### waitForSelector

```cpp
Result<ElementHandle> waitForSelector(const std::string& selector, int timeoutMs = DEFAULT_TIMEOUT_MS);
Result<ElementHandle> waitForSelector(const std::string& selector, const WaitOptions& options);
```

Wait for element to appear in DOM.

**Example:**
```cpp
auto elem = page.waitForSelector("#results");

// With options
WaitOptions opts = WaitOptions::fast(5000);
auto elem = page.waitForSelector("#results", opts);
```

#### waitForVisible

```cpp
Result<ElementHandle> waitForVisible(const std::string& selector, int timeoutMs = DEFAULT_TIMEOUT_MS);
Result<ElementHandle> waitForVisible(const std::string& selector, const WaitOptions& options);
```

Wait for element to be visible.

#### waitForHidden

```cpp
Result<void> waitForHidden(const std::string& selector, int timeoutMs = DEFAULT_TIMEOUT_MS);
Result<void> waitForHidden(const std::string& selector, const WaitOptions& options);
```

Wait for element to disappear.

#### waitForNavigation

```cpp
Result<void> waitForNavigation(int timeoutMs = DEFAULT_TIMEOUT_MS);
Result<void> waitForNavigation(const WaitOptions& options);
```

Wait for navigation to complete.

#### waitForNetworkIdle

```cpp
Result<void> waitForNetworkIdle(int idleTimeMs = 500, int timeoutMs = DEFAULT_TIMEOUT_MS);
Result<void> waitForNetworkIdle(int idleTimeMs, const WaitOptions& options);
```

Wait until no network activity for specified duration.

#### waitForFunction

```cpp
Result<void> waitForFunction(const std::string& expression, int timeoutMs = DEFAULT_TIMEOUT_MS);
Result<void> waitForFunction(const std::string& expression, const WaitOptions& options);
```

Wait for JavaScript expression to return truthy value.

**Example:**
```cpp
page.waitForFunction("window.dataLoaded === true");
page.waitForFunction("document.querySelectorAll('.item').length > 5");
```

#### waitForURL

```cpp
Result<void> waitForURL(const std::string& urlPattern, int timeoutMs = DEFAULT_TIMEOUT_MS);
```

Wait for URL to match pattern.

#### waitForAny

```cpp
Result<std::pair<int, ElementHandle>> waitForAny(const std::vector<std::string>& selectors, int timeoutMs = DEFAULT_TIMEOUT_MS);
```

Wait for any of multiple selectors and return which matched.

**Example:**
```cpp
auto result = page.waitForAny({"#success", "#error", "#timeout"});
if (result) {
    int index = result.value().first;  // Which selector matched
    ElementHandle& elem = result.value().second;
}
```

---

### JavaScript Execution

#### evaluate

```cpp
Result<JsonValue> evaluate(const std::string& expression);
```

Evaluate JavaScript and return result.

**Example:**
```cpp
auto result = page.evaluate("document.querySelectorAll('li').length");
if (result) {
    int count = result.value().getInt();
}

auto data = page.evaluate(R"(
    JSON.stringify({
        title: document.title,
        links: document.querySelectorAll('a').length
    })
)");
```

#### evaluateString / evaluateInt / evaluateBool

```cpp
Result<std::string> evaluateString(const std::string& expression);
Result<int> evaluateInt(const std::string& expression);
Result<bool> evaluateBool(const std::string& expression);
```

Type-specific evaluation helpers.

---

### Screenshots & PDF

#### screenshot

```cpp
Result<std::vector<uint8_t>> screenshot(const ScreenshotOptions& options = {});
```

Take screenshot and return bytes.

#### screenshotToFile

```cpp
Result<void> screenshotToFile(const std::string& path, const ScreenshotOptions& options = {});
```

Take screenshot and save to file.

**Example:**
```cpp
page.screenshotToFile("page.png");

ScreenshotOptions opts;
opts.fullPage = true;
opts.format = "jpeg";
opts.quality = 90;
page.screenshotToFile("full.jpg", opts);
```

#### pdf / pdfToFile

```cpp
Result<std::vector<uint8_t>> pdf();
Result<void> pdfToFile(const std::string& path);
```

Generate PDF (headless Chrome only).

---

### Viewport & Emulation

#### setViewport

```cpp
Result<void> setViewport(int width, int height, double deviceScaleFactor = 1.0);
```

Set viewport size.

#### setUserAgent

```cpp
Result<void> setUserAgent(const std::string& userAgent);
```

Set user agent string.

#### emulateDevice

```cpp
Result<void> emulateDevice(const std::string& deviceName);
```

Emulate a device (iPhone, iPad, etc.).

---

### Cookies

#### getCookies

```cpp
Result<JsonArray> getCookies();
```

Get all cookies.

#### setCookie

```cpp
Result<void> setCookie(const std::string& name, const std::string& value,
                       const std::string& domain = "", const std::string& path = "/");
```

Set a cookie.

#### deleteCookie

```cpp
Result<void> deleteCookie(const std::string& name, const std::string& domain = "");
```

Delete a cookie.

#### clearCookies

```cpp
Result<void> clearCookies();
```

Clear all cookies.

---

### Convenience Methods

#### fillForm

```cpp
Result<void> fillForm(const std::map<std::string, std::string>& fields, int timeoutMs = 30000);
```

Fill multiple form fields at once.

**Example:**
```cpp
page.fillForm({
    {"#firstName", "John"},
    {"#lastName", "Doe"},
    {"#email", "john@example.com"},
    {"#phone", "555-1234"}
});
```

#### fillFormAndSubmit

```cpp
Result<void> fillFormAndSubmit(const std::map<std::string, std::string>& fields,
                               const std::string& submitSelector = "",
                               int timeoutMs = 30000);
```

Fill form and submit (click button or press Enter).

#### getTexts

```cpp
Result<std::vector<std::string>> getTexts(const std::string& selector, int timeoutMs = 30000);
```

Get text content from all matching elements.

#### scrollToElement / scrollBy / scrollTo

```cpp
Result<void> scrollToElement(const std::string& selector, int timeoutMs = 30000);
Result<void> scrollBy(int x, int y);
Result<void> scrollTo(int x, int y);
```

Scrolling operations.

#### isVisible / isEnabled / isChecked

```cpp
Result<bool> isVisible(const std::string& selector);
Result<bool> isEnabled(const std::string& selector);
Result<bool> isChecked(const std::string& selector);
```

Check element state.

#### getComputedStyle

```cpp
Result<std::string> getComputedStyle(const std::string& selector, const std::string& property);
```

Get computed CSS property value.

---

## WaitOptions

Configuration for wait operations.

```cpp
struct WaitOptions {
    int timeoutMs = 30000;          // Maximum wait time
    int pollIntervalMs = 50;        // Initial polling interval
    int maxPollIntervalMs = 200;    // Maximum polling interval (backoff)
    bool visible = false;           // Wait for visibility
    CancellationToken* cancellationToken = nullptr;

    // Factory methods
    static WaitOptions fast(int timeoutMs = 5000);
    static WaitOptions standard(int timeoutMs = 30000);
    static WaitOptions slow(int timeoutMs = 60000);
    static WaitOptions withCancellation(CancellationToken* token, int timeoutMs = 30000);
};
```

---

## NavigateOptions

Configuration for navigation.

```cpp
struct NavigateOptions {
    int timeoutMs = 30000;
    WaitUntil waitUntil = WaitUntil::Load;
    std::string referer;

    // waitUntil values:
    // - Load: Wait for 'load' event (default)
    // - DOMContentLoaded: DOM ready, resources may still load
    // - NetworkIdle0: No network for 500ms
    // - NetworkIdle2: <= 2 connections for 500ms
    // - None: Return immediately
};
```

---

## TypeOptions

Configuration for typing.

```cpp
struct TypeOptions {
    bool clearFirst = true;      // Clear before typing
    bool useKeyEvents = false;   // Individual key events (slower but more compatible)
    int delayMs = 0;             // Delay between keystrokes
};
```

---

## ScreenshotOptions

Configuration for screenshots.

```cpp
struct ScreenshotOptions {
    std::string format = "png";     // "png" or "jpeg"
    int quality = 80;               // JPEG quality (1-100)
    bool fullPage = false;          // Capture entire scrollable page
    bool optimizeForSpeed = true;   // Faster but lower quality
    int timeoutMs = 120000;         // Timeout (screenshots can be slow)
    std::optional<BoundingBox> clip; // Clip to region
};
```

---

## CancellationToken

Cancel long-running operations from another thread.

```cpp
CancellationToken token;

// In another thread
std::thread([&token]() {
    std::this_thread::sleep_for(std::chrono::seconds(5));
    token.cancel();
}).detach();

// Will be cancelled after 5 seconds
page.waitForSelector("#slow-element", WaitOptions::withCancellation(&token));
```

**Methods:**
- `cancel()` - Request cancellation
- `isCancelled()` - Check if cancelled
- `reset()` - Reset for reuse
