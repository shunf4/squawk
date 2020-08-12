cmake_minimum_required(VERSION 3.0)
project(squawkWidgets)

# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)
# Instruct CMake to create code from Qt designer ui files
set(CMAKE_AUTOUIC ON)

# Find the QtWidgets library
find_package(Qt5Widgets CONFIG REQUIRED COMPONENTS Widgets Quick Qml QuickControls2 Core)

add_subdirectory(vcard)

set(squawkWidgets_SRC
  conversation.cpp
  chat.cpp
  room.cpp
  newcontact.cpp
  accounts.cpp
  account.cpp
  joinconference.cpp
)

add_library(squawkWidgets ${squawkWidgets_SRC})

# Use the Widgets module from Qt 5.
target_link_libraries(squawkWidgets vCardUI)
target_link_libraries(squawkWidgets Qt5::Widgets)
target_link_libraries(squawkWidgets Qt5::Qml)
target_link_libraries(squawkWidgets Qt5::QuickControls2)

qt5_use_modules(squawkWidgets Quick Qml QuickControls2 Core Widgets)
