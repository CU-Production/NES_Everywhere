cmake_minimum_required(VERSION 3.31)
project(libnes)

set(CMAKE_CXX_STANDARD 20)

add_library(nes SHARED agnes.c agnes.h)
if (WIN32)
    target_compile_definitions(nes PRIVATE _WIN32 AGNES_DLL AGNES_APP_IMPL)
endif ()