# Contributing to CDP for C++

Thank you for your interest in contributing! This document provides guidelines and information for contributors.

## Code of Conduct

Be respectful and constructive. We welcome contributions from everyone regardless of experience level.

## Getting Started

1. Fork the repository
2. Clone your fork: `git clone https://github.com/your-username/cdp-cpp.git`
3. Create a feature branch: `git checkout -b feature/your-feature`
4. Make your changes
5. Run tests: `cmake --build build --target cdp_torture_test && ./build/Release/cdp_torture_test.exe`
6. Commit with a clear message
7. Push and create a Pull Request

## Development Setup

```bash
# Clone and build
git clone https://github.com/your-username/cdp-cpp.git
cd cdp-cpp
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build .
```

## Code Style

### General Guidelines

- **C++23** standard - use modern features where appropriate
- **No external dependencies** - keep the library self-contained
- **Thread safety** - all public APIs must be thread-safe
- **RAII** - use smart pointers and RAII for resource management

### Naming Conventions

```cpp
// Classes: PascalCase
class CDPClient;
class BrowserContext;

// Functions/Methods: camelCase
void sendCommand();
bool isConnected();

// Variables: camelCase
int connectionTimeout;
std::string lastError;

// Member variables: trailing underscore
class Example {
    int value_;
    std::string name_;
};

// Constants: UPPER_SNAKE_CASE
constexpr int MAX_RETRIES = 3;
constexpr size_t BUFFER_SIZE = 4096;

// Namespaces: lowercase
namespace cdp::quick { }
```

### File Organization

```
include/cdp/           # Public headers
    CDP.hpp            # Main include (umbrella header)
    CDPClient.hpp      # Individual components
    domains/           # CDP domain implementations
    highlevel/         # High-level API

src/                   # Implementation files
    CDPClient.cpp
    ...

examples/              # Example programs
    main.cpp
    ...
```

### Header Structure

```cpp
#pragma once

// Standard library includes (alphabetical)
#include <memory>
#include <string>
#include <vector>

// Project includes (alphabetical)
#include "cdp/CDPConnection.hpp"
#include "cdp/Json.hpp"

namespace cdp {

// Forward declarations
class CDPClient;

// Class definition
class MyClass {
public:
    // Constructors/destructor
    MyClass();
    ~MyClass();

    // Deleted operations
    MyClass(const MyClass&) = delete;
    MyClass& operator=(const MyClass&) = delete;

    // Public methods
    bool doSomething();

private:
    // Private methods
    void helperMethod();

    // Member variables
    int value_;
};

} // namespace cdp
```

### Error Handling

```cpp
// DO: Use typed exception catches
try {
    // code
} catch (const std::exception& e) {
    lastError_ = e.what();
    return false;
}

// DON'T: Bare catch-all
try {
    // code
} catch (...) {  // Bad - loses error information
    return false;
}

// DO: Return Result<T> for operations that can fail
Result<Page> newPage(const std::string& url);

// DO: Document error conditions
/// @throws std::runtime_error if connection fails
void connect();
```

### Threading Guidelines

```cpp
// DO: Use mutex for shared state
mutable std::mutex mutex_;
std::lock_guard<std::mutex> lock(mutex_);

// DO: Use atomic for simple flags
std::atomic<bool> running_{false};

// DON'T: Hold locks while calling callbacks
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto callback = callback_;  // Copy under lock
}
callback();  // Call outside lock

// DO: Document thread safety requirements
/// Thread-safe. Can be called from any thread.
void sendCommand();

/// NOT thread-safe. Must be called from main thread only.
void processEvents();
```

## Testing

### Running Tests

```bash
# Build all tests
cmake --build build --config Release

# Run comprehensive test
./build/Release/cdp_comprehensive.exe

# Run torture test (stress testing)
./build/Release/cdp_torture_test.exe
```

### Writing Tests

When adding new features, add corresponding tests:

1. **Unit tests** - Test individual functions/classes
2. **Integration tests** - Test feature workflows
3. **Stress tests** - Test concurrent/edge cases

Example test structure:

```cpp
void testFeature() {
    // Arrange
    auto browser = cdp::quick::launch();
    auto page = browser->newPage("about:blank");

    // Act
    bool result = page->doSomething();

    // Assert
    if (!result) {
        std::cerr << "[FAIL] doSomething returned false\n";
        return;
    }
    std::cout << "[PASS] doSomething works\n";
}
```

## Pull Request Guidelines

### Before Submitting

- [ ] Code compiles without warnings
- [ ] All existing tests pass
- [ ] New code has corresponding tests
- [ ] Code follows style guidelines
- [ ] Documentation updated if needed
- [ ] Commit messages are clear and descriptive

### PR Description Template

```markdown
## Description
Brief description of changes.

## Type of Change
- [ ] Bug fix
- [ ] New feature
- [ ] Breaking change
- [ ] Documentation update

## Testing
Describe how you tested the changes.

## Checklist
- [ ] Tests pass
- [ ] Code follows style guide
- [ ] Self-review completed
```

### Commit Messages

Use clear, descriptive commit messages:

```
feat: add network request interception support

- Implement Fetch domain handler
- Add FetchAction for continue/fail/fulfill
- Support context-level interception

Closes #123
```

Prefixes:
- `feat:` - New feature
- `fix:` - Bug fix
- `docs:` - Documentation
- `refactor:` - Code refactoring
- `test:` - Test additions/changes
- `chore:` - Build/tooling changes

## Architecture Overview

```
┌─────────────────────────────────────────────────────────────┐
│                      User Application                        │
├─────────────────────────────────────────────────────────────┤
│  QuickStart API      │  High-Level API   │   Low-Level API  │
│  (cdp::quick::)      │  (cdp::Browser)   │   (cdp::CDPClient)│
├─────────────────────────────────────────────────────────────┤
│                        CDPClient                             │
│  ┌─────────────────────────────────────────────────────────┐│
│  │ Domain Handlers (Page, Runtime, Network, Fetch, etc.)   ││
│  └─────────────────────────────────────────────────────────┘│
├─────────────────────────────────────────────────────────────┤
│                      CDPConnection                           │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │ Command Queue │  │ Event Router │  │ Response Map │      │
│  └──────────────┘  └──────────────┘  └──────────────┘      │
├─────────────────────────────────────────────────────────────┤
│                        WebSocket                             │
├─────────────────────────────────────────────────────────────┤
│                     Winsock (Windows)                        │
└─────────────────────────────────────────────────────────────┘
```

## Reporting Issues

When reporting bugs, include:

1. **Environment** - OS version, compiler, Chrome version
2. **Steps to reproduce** - Minimal code example
3. **Expected behavior** - What should happen
4. **Actual behavior** - What actually happens
5. **Error messages** - Full error output

## Questions?

Open an issue with the "question" label or start a discussion.

Thank you for contributing!
