cmake_minimum_required(VERSION 3.0)
project(squawkCORE)

set(CMAKE_AUTOMOC ON)

find_package(Qt5Core CONFIG REQUIRED)
find_package(Qt5Gui CONFIG REQUIRED)
find_package(Qt5Network CONFIG REQUIRED)
find_package(Qt5Xml CONFIG REQUIRED)

find_path(LMDB_INCLUDE_DIR NAMES  lmdb.h PATHS "${LMDB_DIR}/include")

if (UNIX AND NOT APPLE)
    find_library(LMDB_LIBRARIES NAMES liblmdb.a PATHS ${LMDB_DIR})
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
else (UNIX AND NOT APPLE)
    set(LMDB_LIBRARIES "${LMDB_DIR}/lib/liblmdb.a")
endif (UNIX AND NOT APPLE)

message(${LMDB_INCLUDE_DIR})
message(${LMDB_LIBRARIES})

set(squawkCORE_SRC
    squawk.cpp
    account.cpp
    archive.cpp
    rosteritem.cpp
    contact.cpp
    conference.cpp
    storage.cpp
    networkaccess.cpp
    adapterFuctions.cpp
    handlers/messagehandler.cpp
    handlers/rosterhandler.cpp
)

add_subdirectory(passwordStorageEngines)

# Tell CMake to create the helloworld executable
add_library(squawkCORE ${squawkCORE_SRC})

target_include_directories(squawkCORE PUBLIC ${LMDB_INCLUDE_DIR})

if(SYSTEM_QXMPP)
    get_target_property(QXMPP_INTERFACE_INCLUDE_DIRECTORIES QXmpp::QXmpp INTERFACE_INCLUDE_DIRECTORIES)
    target_include_directories(squawkCORE PUBLIC ${QXMPP_INTERFACE_INCLUDE_DIRECTORIES})
endif()

# Use the Widgets module from Qt 5.
target_link_libraries(squawkCORE Qt5::Core)
target_link_libraries(squawkCORE Qt5::Network)
target_link_libraries(squawkCORE Qt5::Gui)
target_link_libraries(squawkCORE Qt5::Xml)
target_link_libraries(squawkCORE qxmpp)
# target_link_libraries(squawkCORE lmdb)
target_link_libraries(squawkCORE ${LMDB_LIBRARIES})
if (UNIX AND NOT APPLE)
    target_link_libraries(squawkCORE Threads::Threads)
endif (UNIX AND NOT APPLE)
target_link_libraries(squawkCORE simpleCrypt)
if (WITH_KWALLET)
    target_link_libraries(squawkCORE kwalletPSE)
endif()
