cmake_minimum_required(VERSION 3.0)
project(simplecrypt LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

find_package(Qt5 COMPONENTS Core REQUIRED)

add_library(simpleCrypt STATIC simplecrypt.cpp simplecrypt.h)

target_link_libraries(simpleCrypt Qt5::Core)
