cmake_minimum_required(VERSION 3.15)
project(skernel)
set(CMAKE_CXX_STANDARD 14)
set(SOURCE_FILES kernel.c)

add_executable(skernel ${SOURCE_FILES})