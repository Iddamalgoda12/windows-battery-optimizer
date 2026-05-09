cmake_minimum_required(VERSION 3.15)

project(BatteryOptimizer)

set(CMAKE_CXX_STANDARD 20)

add_definitions(-DUNICODE -D_UNICODE)

add_executable(
    BatteryOptimizer

    src/main.cpp
    src/process_monitor.cpp
)

target_include_directories(
    BatteryOptimizer
    PRIVATE
        include
        "E:/DSA project/Dev/vcpkg/installed/x64-windows/include"
)