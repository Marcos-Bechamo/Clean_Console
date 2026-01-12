cmake_minimum_required(VERSION 3.16)
project(console_table)

set(CMAKE_CXX_STANDARD 17)

find_package(spdlog REQUIRED)

add_executable(console_table main.cpp)
target_link_libraries(console_table spdlog::spdlog)
