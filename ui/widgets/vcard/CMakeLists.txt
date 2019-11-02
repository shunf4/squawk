cmake_minimum_required(VERSION 3.0)
project(vCardUI)

# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)
# Instruct CMake to create code from Qt designer ui files
set(CMAKE_AUTOUIC ON)

# Find the QtWidgets library
find_package(Qt5Widgets CONFIG REQUIRED)

set(vCardUI_SRC
  vcard.cpp
  emailsmodel.cpp
)

# Tell CMake to create the helloworld executable
add_library(vCardUI ${vCardUI_SRC})

# Use the Widgets module from Qt 5.
target_link_libraries(vCardUI Qt5::Widgets)
