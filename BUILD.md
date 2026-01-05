# Building CDP for C++

## Requirements

### Platform
- **Windows 10/11** - Uses Winsock2 for networking
- **Linux** (Ubuntu 20.04+, Debian 11+, or equivalent) - Uses POSIX sockets

### Compiler

#### Windows
- **MSVC 2022** (v17.0) or later
- **Clang 17+** with MSVC compatibility
- **MinGW-w64** with C++23 support

#### Linux
- **GCC 13+** with C++23 support
- **Clang 17+** with libc++ or libstdc++

### Tools
- **CMake 3.20** or later
- **Chrome/Chromium** browser installed (for running tests)

## Quick Build

```bash
# Clone the repository
git clone https://github.com/ByteAshen/cdp-cpp.git
cd cdp-cpp

# Create build directory
mkdir build
cd build

# Configure
cmake ..

# Build
cmake --build . --config Release
```

## Build Options

### CMake Configuration Options

```bash
# Debug build
cmake .. -DCMAKE_BUILD_TYPE=Debug

# Release build with debug info
cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo

# Specify compiler (Windows)
cmake .. -G "Visual Studio 17 2022" -A x64

# Specify compiler (Linux - use Ninja for faster builds)
cmake .. -G Ninja -DCMAKE_CXX_COMPILER=g++-13

# Specify install prefix (Windows)
cmake .. -DCMAKE_INSTALL_PREFIX="C:/Program Files/CDPLib"

# Specify install prefix (Linux)
cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local
```

### Build Targets

```bash
# Build static library only
cmake --build . --target cdp

# Build shared library (DLL)
cmake --build . --target cdp_shared

# Build specific example
cmake --build . --target cdp_example

# Build all examples
cmake --build .
```

## Installation

### System-Wide Installation

```bash
# Build and install
cmake --build . --config Release
cmake --install . --config Release

# Or with custom prefix
cmake --install . --config Release --prefix "C:/MyLibs/cdp"
```

This installs:
- `lib/cdp.lib` - Static library
- `lib/cdp_shared.lib` - Import library for DLL
- `bin/cdp_shared.dll` - Shared library
- `include/cdp/` - Header files

### Using as Subdirectory

Add to your project:

```cmake
# CMakeLists.txt
add_subdirectory(external/cdp-cpp)
target_link_libraries(your_app PRIVATE cdp)
```

### Using Installed Library

```cmake
# CMakeLists.txt
find_package(CDPLib REQUIRED)
target_link_libraries(your_app PRIVATE CDPLib::cdp)
```

## Linking

### Static Library (Recommended)

```cmake
# Windows
target_link_libraries(your_app PRIVATE cdp ws2_32)

# Linux
target_link_libraries(your_app PRIVATE cdp pthread)
```

### Shared Library (DLL/SO)

```cmake
# Windows
target_link_libraries(your_app PRIVATE cdp_shared ws2_32)

# Linux
target_link_libraries(your_app PRIVATE cdp_shared pthread)
```

When using the shared library:
- **Windows**: Ensure `cdp_shared.dll` is in your PATH or alongside your executable
- **Linux**: Ensure `libcdp.so` is in your `LD_LIBRARY_PATH` or installed system-wide

## Compiler Flags

### MSVC (Windows)

```bash
# Recommended flags for production
cmake .. -DCMAKE_CXX_FLAGS="/W4 /WX /O2 /GL"

# Enable debug symbols and sanitizers
cmake .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="/W4 /RTC1"
```

### GCC/Clang (Linux)

```bash
# Recommended flags for production
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-Wall -Wextra -O3"

# Enable debug symbols and sanitizers
cmake .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-Wall -Wextra -g -fsanitize=address"
```

## Running Examples

After building:

### Windows

```bash
# Navigate to build output
cd build/Release

# Run basic example
./cdp_example.exe

# Run comprehensive test
./cdp_comprehensive.exe

# Run stress test
./cdp_torture_test.exe
```

### Linux

```bash
# Navigate to build output
cd build

# Run basic example
./cdp_example

# Run comprehensive test
./cdp_comprehensive

# Run stress test
./cdp_torture_test
```

**Note:** Chrome must be installed for examples to work. The library auto-detects Chrome location from:

**Windows:**
1. `CHROME_PATH` environment variable
2. Registry (standard Chrome installation)
3. Common installation paths (`Program Files`, `Program Files (x86)`)

**Linux:**
1. `CHROME_PATH` environment variable
2. Standard paths: `/usr/bin/google-chrome`, `/usr/bin/chromium-browser`, `/usr/bin/chromium`
3. Snap paths: `/snap/bin/chromium`

## Troubleshooting

### Chrome Not Found

Set the Chrome path explicitly:

```cpp
cdp::ChromeLaunchOptions opts;
// Windows
opts.chromePath = "C:/Path/To/chrome.exe";
// Linux
opts.chromePath = "/usr/bin/google-chrome";

auto browser = cdp::quick::launch(opts);
```

Or set environment variable:
```bash
# Windows
set CHROME_PATH=C:\Path\To\chrome.exe

# Linux
export CHROME_PATH=/usr/bin/google-chrome
```

### Connection Timeouts

Increase timeout values:

```cpp
cdp::ChromeLaunchOptions opts;
opts.startupWaitMs = 10000;  // 10 seconds

cdp::CDPClientConfig config;
config.connectionTimeoutMs = 30000;  // 30 seconds
```

### Build Errors

**"Cannot find ws2_32.lib"** (Windows)
- Ensure Windows SDK is installed
- Check CMake finds the correct SDK

**"cannot find -lpthread"** (Linux)
- Install pthread development files: `sudo apt install libc6-dev`

**"C++23 features not available"**
- Update your compiler (MSVC 2022 17.0+, Clang 17+, GCC 13+)
- Ensure CMake uses correct standard: `set(CMAKE_CXX_STANDARD 23)`
- On Linux, you may need to install a newer GCC: `sudo apt install g++-13`

### Runtime Errors

**"Failed to connect to Chrome"**
- Check Chrome is installed
- Check no firewall blocking localhost connections
- Try a different debugging port

**"WebSocket handshake failed"**
- Chrome may have crashed or been closed
- Check Chrome's DevTools is accessible at `http://localhost:PORT/json`

## Development Build

For contributors working on the library:

```bash
# Full debug build with all warnings
cmake .. -DCMAKE_BUILD_TYPE=Debug \
         -DCMAKE_CXX_FLAGS="/W4 /WX"

# Build and run tests
cmake --build .
./Debug/cdp_torture_test.exe
./Debug/cdp_comprehensive.exe
```

## Cross-Compilation

Cross-compilation between Windows and Linux is not supported. Each platform uses its native APIs:
- **Windows**: Winsock2, Windows Registry, CreateProcess
- **Linux**: POSIX sockets, /proc filesystem, fork/exec

## CI/CD Integration

Example GitHub Actions workflow:

```yaml
name: Build
on: [push, pull_request]

jobs:
  build-windows:
    runs-on: windows-latest
    steps:
      - uses: actions/checkout@v3

      - name: Configure
        run: cmake -B build -G "Visual Studio 17 2022"

      - name: Build
        run: cmake --build build --config Release

      - name: Test
        run: |
          cd build/Release
          ./cdp_torture_test.exe

  build-linux:
    runs-on: ubuntu-24.04
    steps:
      - uses: actions/checkout@v3

      - name: Install dependencies
        run: |
          sudo apt update
          sudo apt install -y g++-13 cmake ninja-build

      - name: Configure
        run: cmake -B build -G Ninja -DCMAKE_CXX_COMPILER=g++-13

      - name: Build
        run: cmake --build build

      - name: Test
        run: |
          cd build
          ./cdp_torture_test
```

## Package Managers

### vcpkg (Planned)

```bash
vcpkg install cdp-cpp
```

### Conan (Planned)

```bash
conan install cdp-cpp/1.0.0@
```

## Questions?

Open an issue on GitHub if you encounter build problems.
