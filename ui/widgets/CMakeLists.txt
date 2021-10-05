cmake_minimum_required(VERSION 3.0)
project(squawkWidgets)

# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)
# Instruct CMake to create code from Qt designer ui files
set(CMAKE_AUTOUIC ON)

# Find the QtWidgets library
find_package(Qt5Widgets CONFIG REQUIRED)

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

# Add to squawkUI_SRC so that the .ui files are displayed in Qt Creator
qt5_wrap_ui(squawkWidgets_SRC
  account.ui
  accounts.ui
  conversation.ui
  joinconference.ui
  newcontact.ui
  vcard/vcard.ui
)

# Tell CMake to create the helloworld executable
add_library(squawkWidgets ${squawkWidgets_SRC})

# Use the Widgets module from Qt 5.
target_link_libraries(squawkWidgets vCardUI)
target_link_libraries(squawkWidgets Qt5::Widgets)
