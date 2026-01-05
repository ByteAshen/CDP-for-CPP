# CDP for C++

A modern, production-ready C++ library for Chrome DevTools Protocol (CDP) automation.

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![C++23](https://img.shields.io/badge/C%2B%2B-23-blue.svg)](https://isocpp.org/std/the-standard)
[![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux-lightgrey.svg)](https://github.com/ByteAshen/cdp-cpp)

## Features

- **Zero external dependencies** - Only system libraries required (no vcpkg/conan needed)
- **Multiple API levels** - From quick one-liners to full protocol control
- **Thread-safe** - Designed for concurrent operations from the ground up
- **Browser context isolation** - Multiple isolated sessions (incognito-like)
- **Network interception** - Mock APIs, block resources, inject headers
- **Extension loading** - Pre-install Chrome extensions with all permissions
- **Comprehensive domains** - Page, Runtime, Network, DOM, Fetch, Input, and more

## Quick Start

```cpp
#include <cdp/CDP.hpp>
#include <iostream>

int main() {
    // Launch Chrome and get a page
    auto browser = cdp::quick::launch();
    auto page = browser->newPage("https://example.com");

    // Get page info
    std::cout << "Title: " << page->title() << std::endl;
    std::cout << "URL: " << page->url() << std::endl;

    // Execute JavaScript
    std::string result = page->evalString("document.title");

    // Take a screenshot
    page->screenshot("screenshot.png");

    return 0;
}
```

## Installation

### Requirements

- **Windows 10/11** or **Linux** (Ubuntu 20.04+, Debian 11+, or equivalent)
- C++23 compiler (MSVC 2022 17.0+, Clang 17+, GCC 13+)
- CMake 3.20+
- Chrome/Chromium browser installed

### Building from Source

```bash
git clone https://github.com/ByteAshen/cdp-cpp.git
cd cdp-cpp
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

### Using in Your Project

```cmake
# CMakeLists.txt
find_package(CDPLib REQUIRED)
target_link_libraries(your_app PRIVATE cdp)
```

Or add as a subdirectory:

```cmake
add_subdirectory(cdp-cpp)
target_link_libraries(your_app PRIVATE cdp)
```

## API Levels

CDP for C++ provides three API levels to match your needs:

### 1. QuickStart API (Easiest)

Perfect for scripts and simple automation:

```cpp
auto browser = cdp::quick::launch();
auto page = browser->newPage("https://google.com");

page->type("#search", "CDP for C++");
page->click("button[type=submit]");
page->waitForNavigation();

std::cout << page->title() << std::endl;
```

### 2. High-Level API (Recommended)

Full control with convenient abstractions:

```cpp
cdp::ChromeLauncher launcher;
launcher.launch();

cdp::Browser browser(launcher.wsEndpoint());
auto page = browser.newPage();

page.navigate("https://example.com");
page.waitForSelector("#content");

auto element = page.querySelector("h1");
std::cout << element.textContent() << std::endl;
```

### 3. Low-Level API (Full Protocol Access)

Direct CDP protocol access for advanced use cases:

```cpp
cdp::CDPClient client("localhost", 9222);
client.connect("/devtools/page/ABC123");

// Direct protocol commands
auto result = client.Runtime.evaluate("1 + 1");
client.Page.navigate("https://example.com");

// Event handling
client.Page.onLoadEventFired([](double timestamp) {
    std::cout << "Page loaded at: " << timestamp << std::endl;
});
```

## Browser Contexts

Create isolated browser contexts for parallel testing:

```cpp
auto browser = cdp::quick::launch();

// Each context has isolated cookies, storage, etc.
auto ctx1 = browser->newContext();
auto ctx2 = browser->newContext();

auto page1 = ctx1->newPage("https://site.com");
auto page2 = ctx2->newPage("https://site.com");

// Different sessions - cookies don't leak between contexts
page1->setCookie("session", "user1", "site.com");
page2->setCookie("session", "user2", "site.com");
```

## Network Interception

Intercept and modify network requests:

```cpp
auto ctx = browser->newContext();

// Context-level interception (applies to all pages)
ctx->enableFetch([](const cdp::quick::FetchRequest& req,
                    cdp::quick::FetchAction& action) {
    // Block images
    if (req.resourceType == "Image") {
        action.fail("BlockedByClient");
        return true;
    }

    // Mock API responses
    if (req.url.find("/api/") != std::string::npos) {
        action.fulfillJson(200, R"({"mocked": true})");
        return true;
    }

    // Add custom headers
    auto headers = req.getHeaders();
    cdp::HeaderEntry::set(headers, "X-Custom", "value");
    action.continueRequest(headers);
    return true;
});

auto page = ctx->newPage("https://example.com");
```

## Extension Loading

Load Chrome extensions at launch:

```cpp
cdp::ChromeLaunchOptions opts;
opts.extensions = {
    "/path/to/extension1",        // Linux
    "C:/path/to/extension2"       // Windows
};
opts.extensionIncognitoEnabled = true;  // Enable in incognito contexts

auto browser = cdp::quick::launch(opts);
```

## Screenshots and PDF

```cpp
auto page = browser->newPage("https://example.com");

// Screenshot (PNG)
page->screenshot("page.png");

// Full page screenshot
page->screenshot("full.png", true);

// PDF export
auto& client = page->client();
auto pdfData = client.Page.printToPDF();
// Save pdfData to file...
```

## Input Simulation

```cpp
auto page = browser->newPage("https://example.com");

// Typing
page->type("#input", "Hello World");

// Clicking
page->click("#button");

// Keyboard shortcuts
page->client().Input.dispatchKeyEvent("keyDown", 0, 0, "Enter");

// Mouse movement
page->client().Input.dispatchMouseEvent("mouseMoved", 100, 200);
```

## Configuration Options

```cpp
cdp::ChromeLaunchOptions opts;

// Window settings
opts.headless = false;
opts.windowWidth = 1920;
opts.windowHeight = 1080;

// Profile settings
opts.useTempProfile = true;  // Fresh profile each run
// opts.userDataDir = "/home/user/.config/custom-profile";  // Linux
// opts.userDataDir = "C:/custom/profile";                  // Windows

// Network settings
opts.proxyServer = "http://proxy:8080";
opts.ignoreCertErrors = true;

// Chrome flags
opts.extraArgs = {"--disable-gpu", "--no-sandbox"};

auto browser = cdp::quick::launch(opts);
```

## Error Handling

```cpp
auto result = browser->newPage("https://example.com");

if (!result) {
    std::cerr << "Error: " << result.error << std::endl;
    std::cerr << "Code: " << static_cast<int>(result.code) << std::endl;
    return 1;
}

auto page = std::move(result);
// Use page...
```

## Thread Safety

CDP for C++ is designed for multi-threaded use:

```cpp
auto browser = cdp::quick::launch();
std::vector<std::thread> threads;

for (int i = 0; i < 10; ++i) {
    threads.emplace_back([&browser, i] {
        auto ctx = browser->newContext();
        auto page = ctx->newPage("https://example.com/" + std::to_string(i));
        // Each thread has isolated context
        page->screenshot("page_" + std::to_string(i) + ".png");
        ctx->close();
    });
}

for (auto& t : threads) t.join();
```

## Documentation

- [BUILD.md](BUILD.md) - Detailed build instructions
- [CONTRIBUTING.md](CONTRIBUTING.md) - Contribution guidelines
- [CHANGELOG.md](CHANGELOG.md) - Version history

## Acknowledgments

Inspired by:
- [Puppeteer](https://pptr.dev/) - Node.js CDP library
- [Playwright](https://playwright.dev/) - Cross-browser automation
- [Chrome DevTools Protocol](https://chromedevtools.github.io/devtools-protocol/)

## License

MIT License - see [LICENSE](LICENSE) for details.
