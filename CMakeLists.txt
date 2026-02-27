cmake_minimum_required(VERSION 3.16)

project(cppsion)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(cppsion
    src/main.cpp
    src/util/random.cpp
)

# 让编译器能找到 src 目录里的头文件
target_include_directories(cppsion PRIVATE src)