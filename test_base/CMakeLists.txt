cmake_minimum_required(VERSION 3.16)
project(test_base VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED 17)

add_executable(test_buffer test_buffer.cpp)
include_directories(test_buffer ../base)

add_executable(test_ThreadPool test_ThreadPool.cpp)
include_directories(test_ThreadPool ../base)
target_link_libraries(test_ThreadPool pthread)

add_executable(test_timer test_timer.cpp)
include_directories(test_timer ../base)

set(CMAKE_BUILD_TYPE RELEASE)