cmake_minimum_required(VERSION 3.0)
project(squawkUI)

# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)
# Instruct CMake to create code from Qt designer ui files
set(CMAKE_AUTOUIC ON)

# Find the QtWidgets library
find_package(Qt5Widgets CONFIG REQUIRED)
find_package(Qt5DBus CONFIG REQUIRED)

set(squawkUI_SRC
  squawk.cpp
  accounts.cpp
  account.cpp
  models/accounts.cpp
  models/roster.cpp
  models/item.cpp
  models/account.cpp
  models/contact.cpp
  models/presence.cpp
  models/group.cpp
  models/room.cpp
  conversation.cpp
  messageline.cpp
  newcontact.cpp
)

# Tell CMake to create the helloworld executable
add_library(squawkUI ${squawkUI_SRC})

# Use the Widgets module from Qt 5.
target_link_libraries(squawkUI Qt5::Widgets)
target_link_libraries(squawkUI Qt5::DBus)

# Install the executable
install(TARGETS squawkUI DESTINATION lib)
