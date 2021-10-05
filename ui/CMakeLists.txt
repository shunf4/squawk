cmake_minimum_required(VERSION 3.0)
project(squawkUI)

# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)
# Instruct CMake to create code from Qt designer ui files
set(CMAKE_AUTOUIC ON)

# Find the QtWidgets library
find_package(Qt5Widgets CONFIG REQUIRED)
find_package(Qt5DBus CONFIG REQUIRED)

add_subdirectory(widgets)

set(squawkUI_SRC
  squawk.cpp
  models/accounts.cpp
  models/roster.cpp
  models/item.cpp
  models/account.cpp
  models/contact.cpp
  models/presence.cpp
  models/group.cpp
  models/room.cpp
  models/abstractparticipant.cpp
  models/participant.cpp
  models/reference.cpp
  utils/messageline.cpp
  utils//message.cpp
  utils/resizer.cpp
  utils/image.cpp
  utils/flowlayout.cpp
  utils/badge.cpp
  utils/progress.cpp
  utils/comboboxdelegate.cpp
  utils/dropshadoweffect.cpp
)

# Add squawk.ui to squawkUI_SRC so that the .ui files are displayed in Qt Creator
qt5_wrap_ui(squawkUI_SRC
  squawk.ui
)

# Tell CMake to create the helloworld executable
add_library(squawkUI ${squawkUI_SRC})

# Use the Widgets module from Qt 5.
target_link_libraries(squawkUI squawkWidgets)
target_link_libraries(squawkUI Qt5::Widgets)
target_link_libraries(squawkUI Qt5::DBus)
