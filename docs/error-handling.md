# Error Handling

CDP for C++ uses a `Result<T>` type for explicit error handling without exceptions.

## Result<T> Type

All operations that can fail return `Result<T>`:

```cpp
#include <cdp/highlevel/Result.hpp>
using namespace cdp::highlevel;
```

### Checking Success/Failure

```cpp
Result<std::string> result = page.getText("#element");

// Method 1: Boolean check
if (result) {
    std::cout << result.value() << std::endl;
}

// Method 2: Explicit methods
if (result.ok()) {
    // Success
}
if (result.hasError()) {
    // Failure
}
```

### Accessing Values

```cpp
// Throws if error (check first!)
std::string text = result.value();

// Safe: Returns default if error
std::string text = result.valueOr("N/A");

// Safe: Returns nullptr if error
const std::string* ptr = result.value_or_null();
if (ptr) {
    std::cout << *ptr << std::endl;
}
```

### Accessing Errors

```cpp
if (!result) {
    const Error& err = result.error();
    std::cerr << "Code: " << err.code << std::endl;
    std::cerr << "Message: " << err.message << std::endl;
    std::cerr << "Full: " << err.fullMessage() << std::endl;
}

// Safe: Returns nullptr if success
const Error* err = result.error_or_null();
if (err) {
    // Handle error
}
```

### Result<void>

For operations that don't return a value:

```cpp
Result<void> result = page.click("#button");

if (result) {
    // Success
} else {
    std::cerr << result.error().message << std::endl;
}
```

---

## Error Type

```cpp
struct Error {
    int code = 0;
    std::string message;
    std::string context;
    ErrorContext richContext;

    // Category
    ErrorCategory category() const;

    // Type checks
    bool isTimeout() const;
    bool isElementNotFound() const;
    bool isElementStale() const;
    bool isNetwork() const;
    bool isCancelled() const;
    bool isRetryable() const;

    // Full message with context
    std::string fullMessage() const;
};
```

### Error Categories

```cpp
enum class ErrorCategory {
    None,              // No error
    Network,           // Connection issues
    Protocol,          // CDP protocol errors
    Timeout,           // Operation timed out
    ElementNotFound,   // Selector didn't match
    ElementStale,      // Element detached from DOM
    ElementNotVisible, // Element not visible
    ElementNotEnabled, // Element disabled
    Navigation,        // Navigation failed
    JavaScript,        // JS execution error
    InvalidArgument,   // Bad parameter
    NotSupported,      // Operation not supported
    Internal,          // Unexpected error
    Cancelled          // Operation cancelled
};
```

### Error Codes

| Range | Category | Codes |
|-------|----------|-------|
| 1xx | Network | ConnectionFailed (100), ConnectionClosed (101), WebSocketError (102) |
| 2xx | Protocol | ProtocolError (200), InvalidResponse (201), MethodNotFound (202) |
| 3xx | Timeout | Timeout (300), WaitTimeout (301), NavigationTimeout (302), ResponseTimeout (303) |
| 4xx | Element | ElementNotFound (400), ElementStale (401), ElementNotVisible (402), ElementNotEnabled (403), ElementNotInteractable (404), ElementDetached (405), NoSuchFrame (406) |
| 5xx | Navigation | NavigationFailed (500), PageCrashed (501), CertificateError (502), PageNotLoaded (503) |
| 6xx | JavaScript | JavaScriptError (600), JavaScriptException (601), EvaluationFailed (602) |
| 7xx | Argument | InvalidArgument (700), InvalidSelector (701), InvalidUrl (702) |
| 8xx | Other | NotSupported (800), Cancelled (801), Internal (900) |

---

## Error Handling Patterns

### Pattern 1: Check and Handle

```cpp
auto result = page.getText("#price");
if (result) {
    std::cout << "Price: " << result.value() << std::endl;
} else {
    std::cerr << "Error: " << result.error().message << std::endl;
}
```

### Pattern 2: Default Values

```cpp
std::string title = page.getText(".title").valueOr("Untitled");
int count = page.evaluateInt("items.length").valueOr(0);
bool visible = page.isVisible("#modal").valueOr(false);
```

### Pattern 3: Error Type Handling

```cpp
auto result = page.click("#button");
if (!result) {
    const auto& err = result.error();

    if (err.isTimeout()) {
        std::cerr << "Operation timed out" << std::endl;
    } else if (err.isElementNotFound()) {
        std::cerr << "Element not found: " << err.context << std::endl;
    } else if (err.isElementStale()) {
        // Re-query the element
        result = page.click("#button");
    } else {
        std::cerr << "Unexpected error: " << err.fullMessage() << std::endl;
    }
}
```

### Pattern 4: Retryable Errors

```cpp
auto result = page.click("#button");
if (!result && result.error().isRetryable()) {
    // Wait and retry
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    result = page.click("#button");
}
```

### Pattern 5: Map/FlatMap Chaining

```cpp
// Transform successful values
auto upper = page.getText("#name")
    .map([](const std::string& s) { return toUpperCase(s); });

// Chain operations
auto result = page.querySelector("#form")
    .flatMap([](ElementHandle& el) { return el.getAttribute("action"); });
```

### Pattern 6: Optional Conversion

```cpp
// Convert to optional (loses error info)
std::optional<std::string> text = page.getText("#element").toOptional();
if (text) {
    std::cout << *text << std::endl;
}
```

---

## ErrorContext

Rich error context for debugging:

```cpp
struct ErrorContext {
    std::string selector;      // CSS selector
    std::string url;           // URL
    std::string operation;     // Operation name
    std::string cdpMethod;     // CDP method called
    std::string cdpRequest;    // Request JSON
    std::string cdpResponse;   // Response JSON
    std::string frameId;       // Frame ID
    std::string targetId;      // Target ID
    int64_t nodeId = 0;        // DOM node ID
    int attemptNumber = 0;     // Retry attempt
    int maxAttempts = 0;       // Max retries
};
```

### Building Errors with Context

```cpp
Error err(ErrorCode::ElementNotFound, "Element not found");
err.withSelector("#button")
   .withOperation("click")
   .withUrl("https://example.com")
   .withAttempt(2, 3);

std::cerr << err.fullMessage() << std::endl;
// "Element not found [operation=click, selector=#button, url=https://example.com, attempt=2/3]"
```

---

## Creating Custom Results

```cpp
// Success
Result<int> success = Ok(42);
Result<void> voidSuccess = Ok();

// Failure
Result<int> failure = Err<int>("Something went wrong");
Result<int> codeFailure = Err<int>(ErrorCode::Timeout, "Operation timed out");
Result<void> voidFailure = Err("Failed");

// From factory methods
auto result = Result<int>::failure(ErrorCode::InvalidArgument, "Invalid value");
```

---

## Exception Safety

Result<T> provides non-throwing accessors:

```cpp
// These throw if wrong state:
result.value();   // Throws if error
result.error();   // Throws if success

// These never throw:
result.ok();              // Check success
result.hasError();        // Check failure
result.valueOr(default);  // Value or default
result.value_or_null();   // Pointer to value or nullptr
result.error_or_null();   // Pointer to error or nullptr
```

---

## Quick API Error Handling

The quick API (`cdp::quick`) uses simpler error handling:

```cpp
auto browser = cdp::quick::launch();
auto page = browser->newPage("https://example.com");

// Returns false on failure
if (!page->click("#button")) {
    std::cerr << page->lastError() << std::endl;
}

// Returns empty string on failure
std::string text = page->text("#element");
if (text.empty() && !page->exists("#element")) {
    // Element not found
}
```
