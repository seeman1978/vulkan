cmake_minimum_required(VERSION 3.22)
project(Resources_and_Memory)

set(CMAKE_CXX_STANDARD 20)
add_definitions(-DVK_USE_PLATFORM_XCB_KHR)
add_executable(Resources_and_Memory main.cpp)
target_link_libraries(Resources_and_Memory dl xcb)
