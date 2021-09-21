cmake_minimum_required(VERSION 3.4)
project(squawk VERSION 0.1.6 LANGUAGES CXX)

cmake_policy(SET CMP0076 NEW)
cmake_policy(SET CMP0079 NEW)
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

include(GNUInstallDirs)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

add_executable(squawk)
target_include_directories(squawk PRIVATE ${CMAKE_SOURCE_DIR})

option(SYSTEM_QXMPP "Use system qxmpp lib" ON)
option(WITH_KWALLET "Build KWallet support module" ON)
option(WITH_KIO "Build KIO support module" ON)

# Dependencies
## Qt
find_package(Qt5 COMPONENTS Widgets DBus Gui Xml Network Core REQUIRED)
find_package(Boost COMPONENTS)

target_include_directories(squawk PRIVATE ${Boost_INCLUDE_DIRS})
target_include_directories(squawk PRIVATE ${Qt5_INCLUDE_DIRS})
target_include_directories(squawk PRIVATE ${Qt5Widgets_INCLUDE_DIRS})
target_include_directories(squawk PRIVATE ${Qt5DBus_INCLUDE_DIRS})
target_include_directories(squawk PRIVATE ${Qt5Gui_INCLUDE_DIRS})
target_include_directories(squawk PRIVATE ${Qt5Xml_INCLUDE_DIRS})
target_include_directories(squawk PRIVATE ${Qt5Network_INCLUDE_DIRS})
target_include_directories(squawk PRIVATE ${Qt5Core_INCLUDE_DIRS})

## QXmpp
if (SYSTEM_QXMPP)
  find_package(QXmpp CONFIG)

  if (NOT QXmpp_FOUND)
    set(SYSTEM_QXMPP OFF)
    message("QXmpp package wasn't found, trying to build with bundled QXmpp")
  else ()
    message("Building with system QXmpp")
  endif ()
endif ()

if (NOT SYSTEM_QXMPP)
  target_link_libraries(squawk PRIVATE qxmpp)
  add_subdirectory(external/qxmpp)
else ()
  target_link_libraries(squawk PRIVATE QXmpp::QXmpp)
endif ()

## KIO
if (WITH_KIO)
  find_package(KF5KIO CONFIG)

  if (NOT KF5KIO_FOUND)
    set(WITH_KIO OFF)
    message("KIO package wasn't found, KIO support modules wouldn't be built")
  else ()
    target_compile_definitions(squawk PRIVATE WITH_KIO)
    message("Building with support of KIO")
  endif ()
endif ()

## KWallet
if (WITH_KWALLET)
  find_package(KF5Wallet CONFIG)

  if (NOT KF5Wallet_FOUND)
    set(WITH_KWALLET OFF)
    message("KWallet package wasn't found, KWallet support module wouldn't be built")
  else ()
    target_compile_definitions(squawk PRIVATE WITH_KWALLET)
    message("Building with support of KWallet")
  endif ()
endif ()

## Signal (TODO)
# find_package(Signal REQUIRED)

## LMDB
find_package(LMDB REQUIRED)

# Linking
target_link_libraries(squawk PRIVATE Qt5::Core Qt5::Widgets Qt5::DBus Qt5::Network Qt5::Gui Qt5::Xml)
target_link_libraries(squawk PRIVATE lmdb)
target_link_libraries(squawk PRIVATE simpleCrypt)
target_link_libraries(squawk PRIVATE uuid)

# Build type
if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif ()

message("Build type: ${CMAKE_BUILD_TYPE}")

target_compile_options(squawk PRIVATE
  "-Wall;-Wextra"
  "$<$<CONFIG:DEBUG>:-g>"
  "$<$<CONFIG:RELEASE>:-O3>"
  )

add_subdirectory(core)
add_subdirectory(external/simpleCrypt)
add_subdirectory(packaging)
add_subdirectory(plugins)
add_subdirectory(resources)
add_subdirectory(shared)
add_subdirectory(translations)
add_subdirectory(ui)

# Install the executable
install(TARGETS squawk DESTINATION ${CMAKE_INSTALL_BINDIR})
