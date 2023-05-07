cmake_minimum_required(VERSION 3.22)
project(thread_pool)

set(CMAKE_CXX_STANDARD 17)

add_executable(thread_pool main.cpp thread_pool.h thread_pool.cpp)

add_executable(thread_pool_plus test_plus.cpp thread_pool_plus.h)