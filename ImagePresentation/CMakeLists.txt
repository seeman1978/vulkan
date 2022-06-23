cmake_minimum_required(VERSION 3.22)
project(ImagePresentation)

set(CMAKE_CXX_STANDARD 14)
add_definitions(-DVK_USE_PLATFORM_XCB_KHR)
add_executable(ImagePresentation main.cpp)
target_link_libraries(ImagePresentation dl)
