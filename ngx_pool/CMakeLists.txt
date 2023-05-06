cmake_minimum_required(VERSION 3.22)
project(ngx_pool)

set(CMAKE_CXX_STANDARD 14)

add_executable(ngx_pool main.cpp mem_pool.h mem_pool.cpp)