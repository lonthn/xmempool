cmake_minimum_required(VERSION 2.8.12)

project(xmempool)
set(CMAKE_C_FLAGS "-std=c99")
add_executable(test mempool.c test_str.c)