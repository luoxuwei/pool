cmake_minimum_required(VERSION 3.22)
project(ngx_pool)

set(CMAKE_CXX_STANDARD 14)

add_library(ngx_pool mem_pool.h mem_pool.cpp ngx_test.cpp)

add_executable(ngx_pool_test ngx_test.cpp)
target_link_libraries(ngx_pool_test ngx_pool)