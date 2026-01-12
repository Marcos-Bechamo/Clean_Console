cmake_minimum_required(VERSION 3.16)

project(clean_console
    VERSION 1.0
    LANGUAGES CXX
)

# -----------------------------
# C++ standard
# -----------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# -----------------------------
# Find dependencies
# -----------------------------
find_package(spdlog REQUIRED)
find_package(fmt REQUIRED)

# -----------------------------
# Create executable
# -----------------------------
add_executable(clean_console
    main.cc
    src/console.cc
)

# -----------------------------
# Include directories
# -----------------------------
target_include_directories(clean_console
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# -----------------------------
# Link libraries
# -----------------------------
target_link_libraries(clean_console
    PRIVATE
        spdlog::spdlog
        fmt::fmt
)

# -----------------------------
# Warnings (optional but recommended)
# -----------------------------
if (MSVC)
    target_compile_options(clean_console PRIVATE /W4)
else()
    target_compile_options(clean_console PRIVATE -Wall -Wextra -Wpedantic)
endif()
