cmake_minimum_required(VERSION 3.0)
project(squawkCORE)

set(CMAKE_AUTOMOC ON)

find_package(Qt5Core CONFIG REQUIRED)
find_package(Qt5Gui CONFIG REQUIRED)
find_package(Qt5Network CONFIG REQUIRED)
find_package(Qt5Xml CONFIG REQUIRED)

set(squawkCORE_SRC
    squawk.cpp
    account.cpp
    archive.cpp
    rosteritem.cpp
    contact.cpp
    conference.cpp
    storage.cpp
    networkaccess.cpp
)

# Tell CMake to create the helloworld executable
add_library(squawkCORE ${squawkCORE_SRC})

if(SYSTEM_QXMPP)
    find_package(QXmpp CONFIG REQUIRED)
    get_target_property(QXMPP_INTERFACE_INCLUDE_DIRECTORIES QXmpp::QXmpp INTERFACE_INCLUDE_DIRECTORIES)
    target_include_directories(squawkCORE PUBLIC ${QXMPP_INTERFACE_INCLUDE_DIRECTORIES})
endif()


# Use the Widgets module from Qt 5.
target_link_libraries(squawkCORE Qt5::Core)
target_link_libraries(squawkCORE Qt5::Network)
target_link_libraries(squawkCORE Qt5::Gui)
target_link_libraries(squawkCORE Qt5::Xml)
target_link_libraries(squawkCORE qxmpp)
target_link_libraries(squawkCORE lmdb)
