cmake_minimum_required(VERSION 3.0)
project(simplecrypt)

set(CMAKE_AUTOMOC ON)

find_package(Qt5Core CONFIG REQUIRED)

set(simplecrypt_SRC
    simplecrypt.cpp
)

# Tell CMake to create the helloworld executable
add_library(simpleCrypt ${simplecrypt_SRC})

# Use the Widgets module from Qt 5.
target_link_libraries(simpleCrypt Qt5::Core)
