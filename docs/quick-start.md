# Quick Start Guide

This guide will get you up and running with CDP for C++ in minutes.

## Prerequisites

1. **C++23 Compiler**: MSVC 2022, Clang 17+, or GCC 13+
2. **Chrome Browser**: Any recent version of Chrome or Chromium
3. **CMake**: Version 3.16 or higher

## Building the Library

```bash
# Clone and build
cd cdpclean
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

## Your First Script

Create a file called `hello.cpp`:

```cpp
#include <cdp/CDP.hpp>
#include <iostream>

int main() {
    // Launch Chrome automatically
    auto browser = cdp::quick::launch();
    if (!browser) {
        std::cerr << "Failed to launch: " << browser.error << std::endl;
        return 1;
    }

    // Create a new page and navigate
    auto page = browser->newPage("https://example.com");

    // Print the page title
    std::cout << "Title: " << page->title() << std::endl;

    // Take a screenshot
    page->screenshot("example.png");

    // Browser closes automatically when 'browser' goes out of scope
    return 0;
}
```

Compile and run:

```bash
# Windows (MSVC)
cl /std:c++latest /EHsc hello.cpp /I path/to/cdpclean/include /link cdp.lib ws2_32.lib

# Linux/macOS
g++ -std=c++23 hello.cpp -I path/to/cdpclean/include -L path/to/build -lcdp -lpthread -o hello
```

## Common Operations

### Clicking Elements

```cpp
page->click("#button");           // Click by ID
page->click(".submit-btn");       // Click by class
page->click("button[type=submit]"); // Click by attribute
```

### Filling Forms

```cpp
page->type("#email", "user@example.com");
page->type("#password", "secret123");
page->click("#login");
```

### Waiting for Content

```cpp
page->waitFor("#results");        // Wait for element to exist
page->waitVisible("#modal");      // Wait for element to be visible
page->waitNetworkIdle();          // Wait for all network requests to finish
```

### Getting Text

```cpp
std::string title = page->title();
std::string text = page->text("#message");
std::string href = page->attribute("a.link", "href");
```

### Screenshots

```cpp
page->screenshot("page.png");              // Current viewport
page->screenshotFullPage("full.png");      // Entire scrollable page
page->screenshotElement("#chart", "chart.png"); // Specific element
```

### Running JavaScript

```cpp
// Get a value
JsonValue result = page->eval("document.querySelectorAll('li').length");
int count = result.getInt();

// Execute code
page->exec("window.scrollTo(0, document.body.scrollHeight)");
```

## Headless Mode

For server environments or CI/CD:

```cpp
auto browser = cdp::quick::launchHeadless();
```

## Connecting to Existing Chrome

If Chrome is already running with remote debugging:

```bash
# Start Chrome with debugging enabled
chrome --remote-debugging-port=9222
```

```cpp
// Connect to existing instance
auto browser = cdp::quick::connect("localhost", 9222);
```

## Error Handling

The quick API returns `false` or empty strings on failure:

```cpp
if (!page->click("#nonexistent")) {
    std::cerr << "Click failed: " << page->lastError() << std::endl;
}

std::string text = page->text("#element");
if (text.empty() && !page->exists("#element")) {
    std::cerr << "Element not found" << std::endl;
}
```

## Next Steps

- [High-Level API](highlevel-api.md) - For production applications
- [CDP Domains](domains.md) - Low-level CDP access
- [Examples](examples.md) - More code examples
