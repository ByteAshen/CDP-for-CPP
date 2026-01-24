cmake_minimum_required(VERSION 3.20)
project(SecurePreferences VERSION 1.0 LANGUAGES CXX)

# Require C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# MSVC specific flags
if(MSVC)
    add_compile_options(/W4 /permissive- /utf-8)
    # Enable faster builds
    add_compile_options(/MP)
endif()

# Define the executable
add_executable(secure_preferences
    main.cpp
)

# Headers are included via the source files
target_include_directories(secure_preferences PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Link Windows libraries
if(WIN32)
    target_link_libraries(secure_preferences PRIVATE advapi32)
endif()

# Set output directory
set_target_properties(secure_preferences PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
