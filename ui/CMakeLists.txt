cmake_minimum_required(VERSION 3.0)
project(squawkUI)

# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)
# Instruct CMake to create code from Qt designer ui files
set(CMAKE_AUTOUIC ON)

# Find the QtWidgets library
find_package(Qt5 CONFIG REQUIRED COMPONENTS Widgets DBus Core)
find_package(Boost 1.36.0 CONFIG REQUIRED)
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
endif()

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
  models/messagefeed.cpp
  models/element.cpp
  utils/messageline.cpp
  utils//message.cpp
  utils/resizer.cpp
  utils/image.cpp
  utils/flowlayout.cpp
  utils/badge.cpp
  utils/progress.cpp
  utils/comboboxdelegate.cpp
  utils/dropshadoweffect.cpp
  utils/feedview.cpp
  utils/messagedelegate.cpp
)

# Tell CMake to create the helloworld executable
add_library(squawkUI ${squawkUI_SRC})

# Use the Widgets module from Qt 5.
target_link_libraries(squawkUI squawkWidgets)
target_link_libraries(squawkUI Qt5::Widgets)
target_link_libraries(squawkUI Qt5::DBus)
