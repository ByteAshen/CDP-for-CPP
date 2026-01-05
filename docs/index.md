# CDP for C++ Documentation

Welcome to the CDP for C++ documentation. This library provides a modern, production-ready API for browser automation using the Chrome DevTools Protocol.

## Quick Links

| Document | Description |
|----------|-------------|
| [Quick Start](quick-start.md) | Get started in 5 minutes |
| [Examples](examples.md) | Practical code examples |
| [Main README](../README.md) | Full API overview |

## API Reference

| Document | Description |
|----------|-------------|
| [High-Level API](highlevel-api.md) | Page class, wait methods, options |
| [Browser API](browser-api.md) | Browser, contexts, managed pages |
| [CDP Domains](domains.md) | Low-level CDP domain access |
| [Core Components](core.md) | JSON, Base64, utilities |
| [Error Handling](error-handling.md) | Result type, error codes |

## API Levels

CDP for C++ provides three API levels:

### 1. Quick API (`cdp::quick`)

Simplest API for scripts and prototyping:

```cpp
#include <cdp/browser/QuickStart.hpp>

auto browser = cdp::quick::launch();
auto page = browser->newPage("https://example.com");
page->click("#button");
page->screenshot("page.png");
```

### 2. High-Level API (`cdp::highlevel`)

Production API with full error handling:

```cpp
#include <cdp/highlevel/Page.hpp>
#include <cdp/highlevel/Browser.hpp>

using namespace cdp::highlevel;

Browser browser;
browser.connect();

auto managed = browser.newPage();
Page& page = managed->page();

auto result = page.navigate("https://example.com");
if (!result) {
    std::cerr << result.error().message << std::endl;
}
```

### 3. Low-Level API (`cdp`)

Direct CDP protocol access:

```cpp
#include <cdp/protocol/CDPClient.hpp>

CDPClient client;
client.connect();

client.Page.enable();
client.Page.navigate("https://example.com");
client.Runtime.evaluate("document.title");
```

## Common Tasks

| Task | Quick API | High-Level API |
|------|-----------|----------------|
| Navigate | `page->navigate(url)` | `page.navigate(url)` |
| Click | `page->click(selector)` | `page.click(selector)` |
| Type | `page->type(selector, text)` | `page.type(selector, text)` |
| Wait | `page->waitFor(selector)` | `page.waitForSelector(selector)` |
| Screenshot | `page->screenshot(path)` | `page.screenshotToFile(path)` |
| Get text | `page->text(selector)` | `page.getText(selector)` |
| Run JS | `page->eval(expr)` | `page.evaluate(expr)` |

## Installation

```bash
cd cdpclean
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

## Requirements

- C++23 compiler (MSVC 2022+, Clang 17+, GCC 13+)
- CMake 3.16+
- Chrome or Chromium browser

## Thread Safety

- All CDP commands are thread-safe
- Multiple threads can use the same client
- Event callbacks run on a background thread
- Never call sync methods from event handlers

## Support

- GitHub Issues: Report bugs and feature requests
- License: MIT
