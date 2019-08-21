cmake_minimum_required(VERSION 3.0)
project(squawkCORE)

# Instruct CMake to run moc automatically when needed.
set (CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)

find_package(Qt5Widgets CONFIG REQUIRED)
find_package(Qt5Network CONFIG REQUIRED)

set(squawkCORE_SRC
    squawk.cpp
    account.cpp
    archive.cpp
    rosteritem.cpp
    contact.cpp
    conference.cpp
)

# Tell CMake to create the helloworld executable
add_library(squawkCORE ${squawkCORE_SRC})

# Use the Widgets module from Qt 5.
target_link_libraries(squawkCORE Qt5::Core)
target_link_libraries(squawkCORE Qt5::Network)
target_link_libraries(squawkCORE qxmpp)
target_link_libraries(squawkCORE lmdb)

# Install the executable
install(TARGETS squawkCORE DESTINATION lib)
