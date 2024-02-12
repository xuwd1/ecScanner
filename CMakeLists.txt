cmake_minimum_required(VERSION 3.23)
project(bmftohex VERSION 0.1 LANGUAGES CXX C)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


# fetch latest argparse
include(FetchContent)
FetchContent_Declare(
    argparse
    GIT_REPOSITORY https://github.com/p-ranav/argparse.git
)
FetchContent_MakeAvailable(argparse)

FetchContent_Declare(
    lexertk
    GIT_REPOSITORY https://github.com/ArashPartow/lexertk.git
)
FetchContent_MakeAvailable(lexertk)

add_executable(devmem devmem2.c)
add_executable(ecscanner ecsanner.cpp)
target_link_libraries(ecscanner argparse)
