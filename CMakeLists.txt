cmake_minimum_required(VERSION 3.16)

project(cppsion)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(cppsion
    src/main.cpp
    src/util/random.cpp
    src/config/config.cpp
    src/ast/printer.cpp
    src/generator/program_generator.cpp
)

# 让编译器能找到 src 目录里的头文件
target_include_directories(cppsion PRIVATE src)

if (MSVC)
    target_compile_options(cppsion PRIVATE /W4 /permissive-)
else()
    target_compile_options(cppsion PRIVATE -Wall -Wextra -Wpedantic)
endif()