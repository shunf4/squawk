cmake_minimum_required(VERSION 3.0)
project(squawk)

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)
# Instruct CMake to create code from Qt designer ui files
set(CMAKE_AUTOUIC ON)

find_package(Qt5Widgets CONFIG REQUIRED)
set(squawk_SRC
  main.cpp
  signalcatcher.cpp
)

# Tell CMake to create the helloworld executable
add_executable(squawk ${squawk_SRC})
target_link_libraries(squawk Qt5::Widgets)

add_subdirectory(ui)
add_subdirectory(core)

target_link_libraries(squawk squawkUI)
target_link_libraries(squawk squawkCORE)

# Install the executable
install(TARGETS squawk DESTINATION bin)
