cmake_minimum_required(VERSION 3.15)

project(project-faux VERSION 0.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(faux main.cpp Server.cpp FieldMapParser.cpp Modbus.cpp Bacnet.cpp Repl.cpp CommandParser.cpp)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBMODBUS REQUIRED IMPORTED_TARGET libmodbus)
include(FetchContent)

FetchContent_Declare(
    yaml-cpp
    GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
    GIT_TAG        yaml-cpp-0.9.0
)

FetchContent_MakeAvailable(yaml-cpp)

FetchContent_Declare(
  bacnet-stack
  GIT_REPOSITORY https://github.com/bacnet-stack/bacnet-stack.git
  GIT_TAG        bacnet-stack-1.6.1
)

set(BACNET_STACK_BUILD_APPS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(bacnet-stack)

FetchContent_Declare(
    argparse
    GIT_REPOSITORY https://github.com/p-ranav/argparse.git
)
FetchContent_MakeAvailable(argparse)

set(BACNET_SERVER_DIR "${bacnet-stack_SOURCE_DIR}/src/bacnet/basic/server")
target_sources(
    faux
    PRIVATE
    "${BACNET_SERVER_DIR}/bacnet_basic.c"
    "${BACNET_SERVER_DIR}/bacnet_device.c")

target_link_libraries(
    faux
    PRIVATE
    yaml-cpp::yaml-cpp
    PkgConfig::LIBMODBUS
    bacnet-stack::bacnet-stack
    argparse)
