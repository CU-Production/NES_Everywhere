cmake_minimum_required(VERSION 3.31)
project(libnes)

set(CMAKE_CXX_STANDARD 20)

add_library(agnes SHARED agnes.c agnes.h agnes_wrap.c)
if (WIN32)
    target_compile_definitions(agnes PRIVATE _WIN32 AGNES_DLL AGNES_APP_IMPL)
endif ()