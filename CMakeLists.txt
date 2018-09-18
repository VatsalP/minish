cmake_minimum_required(VERSION 3.12)
project(minish C)

set(CMAKE_C_STANDARD 11)

add_executable(minish minish.c minish.h)
target_link_libraries(minish readline)