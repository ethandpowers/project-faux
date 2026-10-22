cmake_minimum_required(VERSION 3.15)

project(project-faux VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(faux main.cpp FieldMapParser.cpp Modbus.cpp)

find_package(yaml-cpp REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBMODBUS REQUIRED IMPORTED_TARGET libmodbus)

target_link_libraries(faux PRIVATE yaml-cpp::yaml-cpp PkgConfig::LIBMODBUS)
