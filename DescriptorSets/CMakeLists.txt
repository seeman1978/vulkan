cmake_minimum_required(VERSION 3.23)
project(DescriptorSets)

set(CMAKE_CXX_STANDARD 20)
add_definitions(-DVK_USE_PLATFORM_XCB_KHR)
add_executable(DescriptorSets main.cpp)
target_link_libraries(DescriptorSets dl xcb)
