cmake_minimum_required(VERSION 3.0)
project(squawk)

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)
# Instruct CMake to create code from Qt designer ui files
set(CMAKE_AUTOUIC ON)

set(CMAKE_AUTORCC ON)

find_package(Qt5Widgets CONFIG REQUIRED)
set(squawk_SRC
  main.cpp
  global.cpp
  exception.cpp
  signalcatcher.cpp
)

add_executable(squawk ${squawk_SRC} resources/resources.qrc)
target_link_libraries(squawk Qt5::Widgets)

add_subdirectory(ui)
add_subdirectory(core)
add_subdirectory(external/qxmpp)

target_link_libraries(squawk squawkUI)
target_link_libraries(squawk squawkCORE)
target_link_libraries(squawk uuid)

# Install the executable
install(TARGETS squawk DESTINATION bin)
