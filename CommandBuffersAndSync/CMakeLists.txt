cmake_minimum_required(VERSION 3.22)
project(CommandBuffersAndSync)

set(CMAKE_CXX_STANDARD 20)
add_definitions(-DVK_USE_PLATFORM_XCB_KHR)
add_executable(CommandBuffersAndSync main.cpp)
target_link_libraries(CommandBuffersAndSync dl xcb)
