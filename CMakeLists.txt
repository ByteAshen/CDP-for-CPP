cmake_minimum_required(VERSION 3.20)
project(CDPLib VERSION 1.0.0 LANGUAGES CXX)

# Project Information
set(CDP_PROJECT_DESCRIPTION "A modern C++ library for Chrome DevTools Protocol automation")
set(CDP_PROJECT_HOMEPAGE_URL "https://github.com/ByteAshen/CDP-for-CPP")


# Platform Detection
if(WIN32)
    set(CDP_PLATFORM "windows")
    set(CDP_PLATFORM_SOURCES
        src/net/Socket_win32.cpp
        src/browser/ChromeLauncher.cpp
    )
    set(CDP_PLATFORM_LIBS ws2_32 version shell32)
elseif(UNIX AND NOT APPLE)
    set(CDP_PLATFORM "linux")
    set(CDP_PLATFORM_SOURCES
        src/net/Socket_posix.cpp
        src/browser/ChromeLauncher_posix.cpp
    )
    set(CDP_PLATFORM_LIBS pthread)
elseif(APPLE)
    set(CDP_PLATFORM "macos")
    set(CDP_PLATFORM_SOURCES
        src/net/Socket_posix.cpp
        src/browser/ChromeLauncher_posix.cpp
    )
    set(CDP_PLATFORM_LIBS pthread)
else()
    message(FATAL_ERROR "Unsupported platform")
endif()

message(STATUS "Building for platform: ${CDP_PLATFORM}")

# Build Options
option(CDP_BUILD_EXAMPLES "Build example programs" ON)
option(CDP_ENABLE_WARNINGS "Enable strict compiler warnings" ON)
option(CDP_WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)
option(CDP_ENABLE_ASAN "Enable Address Sanitizer (Debug builds only)" OFF)
option(CDP_ENABLE_LTO "Enable Link-Time Optimization (Release builds only)" OFF)

# Build Configuration
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Position Independent Code for shared libraries
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# MSVC-specific: Enable large object file format
# Required for translation units with many symbols (e.g., large test files)
if(MSVC)
    add_compile_options(/bigobj)
endif()

# Compiler Warnings
if(CDP_ENABLE_WARNINGS)
    if(MSVC)
        add_compile_options(
            /W4             # High warning level
            /permissive-    # Strict conformance mode
            /Zc:__cplusplus # Correct __cplusplus macro
            /Zc:preprocessor # Standard-conforming preprocessor
            /utf-8          # Source and execution charset as UTF-8
        )
        if(CDP_WARNINGS_AS_ERRORS)
            add_compile_options(/WX)
        endif()
        # Disable specific warnings that are noisy but not useful
        add_compile_options(
            /wd4100         # Unreferenced formal parameter
            /wd4127         # Conditional expression is constant
        )
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        add_compile_options(
            -Wall
            -Wextra
            -Wpedantic
            -Wconversion
            -Wsign-conversion
            -Wcast-align
            -Wformat=2
            -Wunused
            -Wnull-dereference
        )
        # GCC-specific: disable warning for "Type member" pattern used in CDPClient
        # This pattern (e.g., "Page Page{connection_}") is valid C++ but GCC warns
        if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
            add_compile_options(-Wno-changes-meaning)
        endif()
        if(CDP_WARNINGS_AS_ERRORS)
            add_compile_options(-Werror)
        endif()
    endif()
endif()

# Sanitizers (Debug builds only)
if(CDP_ENABLE_ASAN AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    if(MSVC)
        add_compile_options(/fsanitize=address)
        add_link_options(/fsanitize=address)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
        add_link_options(-fsanitize=address)
    endif()
endif()

# Link-Time Optimization (Release builds only)
if(CDP_ENABLE_LTO AND CMAKE_BUILD_TYPE STREQUAL "Release")
    include(CheckIPOSupported)
    check_ipo_supported(RESULT LTO_SUPPORTED OUTPUT LTO_ERROR)
    if(LTO_SUPPORTED)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
        message(STATUS "Link-Time Optimization enabled")
    else()
        message(WARNING "LTO not supported: ${LTO_ERROR}")
    endif()
endif()

# Generate version header
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Version.hpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/include/cdp/Version.hpp"
    @ONLY
)

# Library Sources (organized by component)
set(CDP_SOURCES
    # Core utilities
    src/core/Json.cpp
    src/core/Base64.cpp
    src/core/SHA1.cpp

    # Networking layer (platform-agnostic)
    src/net/HttpClient.cpp
    src/net/WebSocket.cpp

    # CDP protocol
    src/protocol/CDPConnection.cpp
    src/protocol/CDPClient.cpp

    # Browser management (platform-agnostic parts)
    src/browser/QuickStart.cpp

    # High-level API
    src/highlevel/Page.cpp
    src/highlevel/ElementHandle.cpp
    src/highlevel/NetworkInterceptor.cpp
    src/highlevel/Browser.cpp

    # Platform-specific sources
    ${CDP_PLATFORM_SOURCES}
)

# Static Library
add_library(cdp STATIC ${CDP_SOURCES})
add_library(CDPLib::cdp ALIAS cdp)

target_include_directories(cdp
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(cdp PRIVATE ${CDP_PLATFORM_LIBS})

target_compile_features(cdp PUBLIC cxx_std_23)

# Shared Library (DLL)
add_library(cdp_shared SHARED ${CDP_SOURCES})
add_library(CDPLib::cdp_shared ALIAS cdp_shared)

target_include_directories(cdp_shared
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(cdp_shared PRIVATE ${CDP_PLATFORM_LIBS})
target_compile_definitions(cdp_shared PRIVATE CDP_EXPORTS)
target_compile_features(cdp_shared PUBLIC cxx_std_23)

set_target_properties(cdp_shared PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    OUTPUT_NAME cdp
)

# Examples
if(CDP_BUILD_EXAMPLES)
    # Basic example
    add_executable(cdp_example examples/main.cpp)
    target_link_libraries(cdp_example PRIVATE cdp)

    # Comprehensive API test
    add_executable(cdp_comprehensive examples/comprehensive_test.cpp)
    target_link_libraries(cdp_comprehensive PRIVATE cdp)

    # Torture test
    add_executable(cdp_torture_test examples/torture_test.cpp)
    target_link_libraries(cdp_torture_test PRIVATE cdp)

    # High-level test test
    add_executable(cdp_highlevel_test examples/highlevel_test.cpp)
    target_link_libraries(cdp_highlevel_test PRIVATE cdp)
endif()

# Installation
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# Install libraries
install(TARGETS cdp cdp_shared
    EXPORT CDPLibTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Install headers
install(DIRECTORY include/cdp
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Install generated version header
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/include/cdp/Version.hpp"
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/cdp
)

# Generate and install CMake config files
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/CDPLibConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CDPLibConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/CDPLibConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/CDPLib
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/CDPLibConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/CDPLibConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/CDPLib
)

install(EXPORT CDPLibTargets
    FILE CDPLibTargets.cmake
    NAMESPACE CDPLib::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/CDPLib
)

# Package Information
set(CPACK_PACKAGE_NAME "CDPLib")
set(CPACK_PACKAGE_VENDOR "CDP for C++ Contributors")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Chrome DevTools Protocol library for C++")
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

include(CPack)
