cmake_minimum_required(VERSION 3.31)
project(winhook)

set(CMAKE_CXX_STANDARD 20)

add_library(HookNotepad SHARED HookNotepad.cpp agnes.c)

add_executable(Injector Injector.cpp)
add_dependencies(Injector HookNotepad)
