cmake_minimum_required(VERSION 3.22)
project(ConnectWithVulkanLoadLibrary)

set(CMAKE_CXX_STANDARD 20)

add_executable(ConnectWithVulkanLoadLibrary main.cpp)
target_link_libraries(ConnectWithVulkanLoadLibrary dl)