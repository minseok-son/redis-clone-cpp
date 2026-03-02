cmake_minimum_required(VERSION 3.10)
project(RedisClone)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 2. Add all your source files from the src directory
add_executable(server 
    main.cpp 
    src/RespParser.cpp
    src/RedisServer.cpp
    src/CommandRegistry.cpp
)

# 1. Tell CMake where to look for .h files
# This allows you to use #include "RespParser.h" instead of #include "include/RespParser.h"
target_include_directories(server PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)


# Optional: Add warnings and optimization
target_compile_options(server PRIVATE -Wall -Wextra -O2)