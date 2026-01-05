# Examples

Practical examples for common browser automation tasks.

## Table of Contents

- [Basic Navigation](#basic-navigation)
- [Form Automation](#form-automation)
- [Web Scraping](#web-scraping)
- [Screenshots & PDF](#screenshots--pdf)
- [Request Interception](#request-interception)
- [Authentication](#authentication)
- [Multiple Pages](#multiple-pages)
- [Device Emulation](#device-emulation)
- [Waiting Strategies](#waiting-strategies)
- [Error Handling](#error-handling)
- [Performance](#performance)

---

## Basic Navigation

### Simple Page Load

```cpp
#include <cdp/CDP.hpp>
#include <iostream>

int main() {
    auto browser = cdp::quick::launch();
    if (!browser) return 1;

    auto page = browser->newPage("https://example.com");

    std::cout << "URL: " << page->url() << std::endl;
    std::cout << "Title: " << page->title() << std::endl;

    return 0;
}
```

### Navigation with History

```cpp
auto page = browser->newPage();

page->navigate("https://google.com");
page->navigate("https://github.com");
page->navigate("https://example.com");

page->back();   // Now at github.com
page->back();   // Now at google.com
page->forward(); // Now at github.com
```

### Wait for Specific Condition

```cpp
page->navigate("https://example.com");

// Wait for DOM ready
page->waitFor("#main-content");

// Wait for network idle (SPA apps)
page->waitNetworkIdle();

// Wait for specific URL
page->waitFor("https://example.com/dashboard");
```

---

## Form Automation

### Login Form

```cpp
auto page = browser->newPage("https://example.com/login");

page->type("#email", "user@example.com");
page->type("#password", "secret123");
page->click("#login-button");

page->waitNavigation();

if (page->exists(".dashboard")) {
    std::cout << "Login successful!" << std::endl;
}
```

### Complex Form

```cpp
using namespace cdp::highlevel;

CDPClient client;
client.connect();
Page page(client);

page.navigate("https://example.com/form");

// Fill multiple fields
page.fillForm({
    {"#firstName", "John"},
    {"#lastName", "Doe"},
    {"#email", "john@example.com"},
    {"#phone", "555-1234"}
});

// Select dropdown
page.select("#country", "US");

// Check checkbox
page.check("#terms", true);

// Upload file
page.uploadFile("#resume", {"path/to/resume.pdf"});

// Submit
page.click("#submit");
```

### Dynamic Form with AJAX Validation

```cpp
page->type("#username", "johndoe");

// Wait for async validation
page->waitFor(".validation-success");

page->type("#email", "john@example.com");
page->waitFor(".email-validated");

page->click("#next-step");
page->waitFor("#step-2");
```

---

## Web Scraping

### Extract Text Content

```cpp
auto page = browser->newPage("https://news.example.com");
page->waitFor(".article");

// Get single element text
std::string headline = page->text("h1.headline");

// Get multiple elements
JsonValue items = page->eval(R"(
    Array.from(document.querySelectorAll('.article')).map(el => ({
        title: el.querySelector('h2').textContent,
        date: el.querySelector('.date').textContent,
        summary: el.querySelector('.summary').textContent
    }))
)");

for (const auto& article : items.asArray()) {
    std::cout << article["title"].getString() << std::endl;
}
```

### Extract Links

```cpp
JsonValue links = page->eval(R"(
    Array.from(document.querySelectorAll('a'))
         .map(a => ({ text: a.textContent, href: a.href }))
         .filter(l => l.href.startsWith('http'))
)");
```

### Infinite Scroll

```cpp
auto page = browser->newPage("https://example.com/feed");

int previousCount = 0;
int attempts = 0;

while (attempts < 10) {
    // Scroll to bottom
    page->exec("window.scrollTo(0, document.body.scrollHeight)");

    // Wait for new content
    std::this_thread::sleep_for(std::chrono::seconds(2));

    int currentCount = page->eval("document.querySelectorAll('.item').length").getInt();

    if (currentCount == previousCount) {
        attempts++;
    } else {
        attempts = 0;
        previousCount = currentCount;
    }
}

std::cout << "Loaded " << previousCount << " items" << std::endl;
```

### Pagination

```cpp
std::vector<std::string> allTitles;

while (true) {
    // Extract from current page
    JsonValue titles = page->eval(R"(
        Array.from(document.querySelectorAll('.item-title'))
             .map(el => el.textContent)
    )");

    for (const auto& title : titles.asArray()) {
        allTitles.push_back(title.getString());
    }

    // Check for next page
    if (!page->exists(".next-page:not(.disabled)")) {
        break;
    }

    page->click(".next-page");
    page->waitNavigation();
}
```

---

## Screenshots & PDF

### Viewport Screenshot

```cpp
page->setViewport(1920, 1080);
page->navigate("https://example.com");
page->screenshot("screenshot.png");
```

### Full Page Screenshot

```cpp
page->screenshotFullPage("full-page.png");
```

### Element Screenshot

```cpp
page->screenshotElement("#chart", "chart.png");
```

### Multiple Viewport Sizes

```cpp
std::vector<std::pair<int, int>> viewports = {
    {1920, 1080},  // Desktop
    {1366, 768},   // Laptop
    {768, 1024},   // Tablet portrait
    {375, 812}     // iPhone X
};

for (auto [width, height] : viewports) {
    page->setViewport(width, height);
    page->screenshot("screenshot-" + std::to_string(width) + "x" + std::to_string(height) + ".png");
}
```

### PDF Generation

```cpp
auto browser = cdp::quick::launchHeadless();
auto page = browser->newPage("https://example.com/report");

page->waitNetworkIdle();
page->pdf("report.pdf");
```

---

## Request Interception

### Block Resources

```cpp
browser->enableFetch([](const FetchRequest& req, FetchAction& action) {
    // Block images and ads
    if (req.resourceType == "Image" ||
        req.url.find("ads.") != std::string::npos) {
        action.block();
        return true;
    }
    return false;  // Continue normally
});
```

### Modify Headers

```cpp
browser->enableFetch([](const FetchRequest& req, FetchAction& action) {
    auto headers = req.getHeaders();
    HeaderEntry::set(headers, "Authorization", "Bearer my-token");
    HeaderEntry::set(headers, "X-Custom-Header", "custom-value");
    action.continueRequest(headers);
    return true;
});
```

### Mock API Response

```cpp
browser->enableFetch([](const FetchRequest& req, FetchAction& action) {
    if (req.url.find("/api/users") != std::string::npos) {
        action.fulfillJson(200, R"([
            {"id": 1, "name": "Mock User 1"},
            {"id": 2, "name": "Mock User 2"}
        ])");
        return true;
    }
    return false;
});
```

### Log All Requests

```cpp
browser->enableFetch([](const FetchRequest& req, FetchAction& action) {
    std::cout << req.method << " " << req.url << std::endl;
    action.continueRequest();
    return true;
});
```

---

## Authentication

### Basic Auth via URL

```cpp
page->navigate("https://user:password@example.com/protected");
```

### Basic Auth via Fetch Interception

```cpp
browser->enableFetch([](const FetchRequest& req, FetchAction& action) {
    auto headers = req.getHeaders();
    std::string credentials = base64Encode("user:password");
    HeaderEntry::set(headers, "Authorization", "Basic " + credentials);
    action.continueRequest(headers);
    return true;
});
```

### Login with Cookies

```cpp
page->navigate("https://example.com");
page->setCookie("session", "abc123xyz", "example.com");
page->navigate("https://example.com/dashboard");
```

### OAuth Flow

```cpp
page->navigate("https://example.com/oauth/login");
page->click("#google-login");

// Wait for redirect to Google
page->waitFor("input[type='email']");
page->type("input[type='email']", "user@gmail.com");
page->click("#next");

page->waitFor("input[type='password']");
page->type("input[type='password']", "password");
page->click("#submit");

// Wait for redirect back
page->waitFor("https://example.com/callback");
page->waitFor(".dashboard");
```

---

## Multiple Pages

### Parallel Pages

```cpp
auto browser = cdp::quick::launch();

auto page1 = browser->newPage("https://google.com");
auto page2 = browser->newPage("https://github.com");
auto page3 = browser->newPage("https://example.com");

// Work with multiple pages
std::cout << "Page 1: " << page1->title() << std::endl;
std::cout << "Page 2: " << page2->title() << std::endl;
std::cout << "Page 3: " << page3->title() << std::endl;
```

### Incognito Context

```cpp
// Regular context
auto page1 = browser->newPage("https://example.com");
page1->setCookie("session", "regular", "example.com");

// Incognito context - isolated cookies
auto context = browser->newContext();
auto page2 = context->newPage("https://example.com");
// page2 does NOT see the cookie from page1

page2->setCookie("session", "incognito", "example.com");
// page1 does NOT see this cookie
```

### Multiple Contexts with Proxies

```cpp
// Direct connection
auto directContext = browser->defaultContext();
auto directPage = directContext.newPage("https://whatsmyip.com");

// Via proxy
ContextOptions proxyOpts;
proxyOpts.proxyServer = "http://proxy.example.com:8080";
proxyOpts.proxyUsername = "user";
proxyOpts.proxyPassword = "pass";

auto proxyContext = browser->newContext(proxyOpts);
auto proxyPage = proxyContext->newPage("https://whatsmyip.com");

// Compare IPs
std::cout << "Direct IP: " << directPage->text("#ip") << std::endl;
std::cout << "Proxy IP: " << proxyPage->text("#ip") << std::endl;
```

---

## Device Emulation

### Mobile Device

```cpp
page->setViewport(375, 812, 3.0);  // iPhone X
page->setUserAgent("Mozilla/5.0 (iPhone; CPU iPhone OS 14_0...)");
page->navigate("https://example.com");
```

### Predefined Devices

```cpp
page->emulateDevice("iPhone 12");
page->emulateDevice("iPad Pro");
page->emulateDevice("Pixel 5");
```

### Geolocation

```cpp
using namespace cdp;

CDPClient client;
client.connect();

// San Francisco
client.Emulation.setGeolocationOverride(37.7749, -122.4194, 100);

Page page(client);
page.navigate("https://maps.google.com");
```

### Dark Mode

```cpp
client.Emulation.setEmulatedMedia("screen", {
    {"prefers-color-scheme", "dark"}
});
```

---

## Waiting Strategies

### Element Presence

```cpp
// Wait for element to exist in DOM
page->waitFor("#element");

// With timeout
page->waitFor("#element", 5000);  // 5 seconds
```

### Element Visibility

```cpp
// Wait for element to be visible
page->waitVisible("#modal");

// Wait for element to disappear
page->waitHidden("#loading-spinner");
```

### Network Idle

```cpp
// Wait for no network activity
page->waitNetworkIdle();

// Custom idle time
page->waitNetworkIdle(1000);  // 1 second of silence
```

### Custom Conditions

```cpp
using namespace cdp::highlevel;

Page page(client);

// Wait for JavaScript condition
page.waitForFunction("window.dataLoaded === true");

// Wait for element count
page.waitForFunction("document.querySelectorAll('.item').length >= 10");

// Wait for any of multiple conditions
auto result = page.waitForAny({
    "#success-message",
    "#error-message",
    "#timeout-message"
});

switch (result.value().first) {
    case 0: std::cout << "Success!" << std::endl; break;
    case 1: std::cout << "Error!" << std::endl; break;
    case 2: std::cout << "Timeout!" << std::endl; break;
}
```

### Cancellation

```cpp
CancellationToken token;

// Cancel from another thread after 5 seconds
std::thread([&token]() {
    std::this_thread::sleep_for(std::chrono::seconds(5));
    token.cancel();
}).detach();

auto result = page.waitForSelector("#slow-element",
    WaitOptions::withCancellation(&token, 60000));

if (!result && result.error().isCancelled()) {
    std::cout << "Wait was cancelled" << std::endl;
}
```

---

## Error Handling

### Graceful Degradation

```cpp
auto title = page.getText("h1.title").valueOr("Untitled");
auto price = page.getText(".price").valueOr("N/A");
auto rating = page.evaluateInt("parseInt(document.querySelector('.rating').dataset.value)").valueOr(0);
```

### Retry Logic

```cpp
int maxAttempts = 3;
for (int attempt = 1; attempt <= maxAttempts; attempt++) {
    auto result = page.click("#flaky-button");

    if (result) {
        break;
    }

    if (result.error().isRetryable() && attempt < maxAttempts) {
        std::cout << "Retry " << attempt << "/" << maxAttempts << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(500 * attempt));
    } else {
        std::cerr << "Failed: " << result.error().message << std::endl;
        break;
    }
}
```

### Error Type Handling

```cpp
auto result = page.click("#element");
if (!result) {
    const auto& err = result.error();

    if (err.isElementNotFound()) {
        // Try alternative selector
        result = page.click("[data-testid='element']");
    } else if (err.isTimeout()) {
        // Increase timeout and retry
        result = page.click("#element", 60000);
    } else if (err.isNetwork()) {
        // Reconnect and retry
        page.client().connect();
        result = page.click("#element");
    }
}
```

---

## Performance

### Disable Images for Speed

```cpp
browser->enableFetch([](const FetchRequest& req, FetchAction& action) {
    if (req.resourceType == "Image") {
        action.block();
        return true;
    }
    return false;
});
```

### Parallel Scraping

```cpp
auto browser = cdp::quick::launch();

std::vector<std::string> urls = {
    "https://example.com/page1",
    "https://example.com/page2",
    "https://example.com/page3"
};

std::vector<std::future<std::string>> futures;
std::mutex resultMutex;
std::vector<std::string> results;

for (const auto& url : urls) {
    futures.push_back(std::async(std::launch::async, [&browser, url]() {
        auto page = browser->newPage(url);
        return page->text("h1");
    }));
}

for (auto& future : futures) {
    results.push_back(future.get());
}
```

### Reuse Browser Instance

```cpp
auto browser = cdp::quick::launch();

for (const auto& url : urls) {
    auto page = browser->newPage(url);
    // Process page
    page->close();  // Close page but keep browser
}
```

### Headless Mode

```cpp
// Headless is faster for non-visual tasks
auto browser = cdp::quick::launchHeadless();
```
