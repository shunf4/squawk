cmake_minimum_required(VERSION 3.3)
project(squawkUIUtils)

# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)
# Instruct CMake to create code from Qt designer ui files
set(CMAKE_AUTOUIC ON)

# Find the QtWidgets library
find_package(Qt5 CONFIG REQUIRED COMPONENTS Widgets Core)

set(squawkUIUtils_SRC
#  messageline.cpp
#  message.cpp
  resizer.cpp
#  image.cpp
  flowlayout.cpp
  badge.cpp
  progress.cpp
  comboboxdelegate.cpp
  feedview.cpp
  messagedelegate.cpp
  exponentialblur.cpp
  shadowoverlay.cpp
)

# Tell CMake to create the helloworld executable
add_library(squawkUIUtils STATIC ${squawkUIUtils_SRC})

# Use the Widgets module from Qt 5.
target_link_libraries(squawkUIUtils squawkWidgets)
target_link_libraries(squawkUIUtils Qt5::Widgets)
