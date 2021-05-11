cmake_minimum_required(VERSION 3.4)
project(squawk VERSION 0.1.6 LANGUAGES CXX)

cmake_policy(SET CMP0076 NEW)
cmake_policy(SET CMP0079 NEW)
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

add_executable(squawk)
target_include_directories(squawk PRIVATE ${CMAKE_SOURCE_DIR})
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

include(GNUInstallDirs)

option(SYSTEM_QXMPP "Use system qxmpp lib" ON)
option(WITH_KWALLET "Build KWallet support module" ON)
option(WITH_KIO "Build KIO support module" ON)

# Dependencies

## Qt
find_package(Qt5Widgets CONFIG REQUIRED COMPONENTS Widgets DBus Core)
find_package(Qt5Core CONFIG REQUIRED)
find_package(Qt5Gui CONFIG REQUIRED)
find_package(Qt5Network CONFIG REQUIRED)
find_package(Qt5Xml CONFIG REQUIRED)
find_package(Qt5LinguistTools)

find_package(LMDB REQUIRED)
find_package(Signal REQUIRED)

## QXmpp
if (SYSTEM_QXMPP)
  find_package(QXmpp CONFIG)

  if (NOT QXmpp_FOUND)
    set(SYSTEM_QXMPP OFF)
    message("QXmpp package wasn't found, trying to build with bundled QXmpp")
  else()
    message("Building with system QXmpp")
  endif()
endif()

if(NOT SYSTEM_QXMPP)
  add_subdirectory(external/qxmpp)
endif()

## KIO
if (WITH_KIO)
  find_package(KF5KIO CONFIG)

  if (NOT KF5KIO_FOUND)
    set(WITH_KIO OFF)
    message("KIO package wasn't found, KIO support modules wouldn't be built")
  else()
    add_definitions(-DWITH_KIO)
    message("Building with support of KIO")
  endif()
endif()

## KWallet
if (WITH_KWALLET)
  find_package(KF5Wallet CONFIG)

  if (NOT KF5Wallet_FOUND)
    set(WITH_KWALLET OFF)
    message("KWallet package wasn't found, KWallet support module wouldn't be built")
  else()
    add_definitions(-DWITH_KWALLET)
    message("Building with support of KWallet")
  endif()
endif()

# Linking
target_link_libraries(squawk PRIVATE Qt5::Widgets)
target_link_libraries(squawk PRIVATE Qt5::DBus)
target_link_libraries(squawk PRIVATE Qt5::Network)
target_link_libraries(squawk PRIVATE Qt5::Gui)
target_link_libraries(squawk PRIVATE Qt5::Xml)
target_link_libraries(squawk PRIVATE qxmpp)
target_link_libraries(squawk PRIVATE lmdb)
target_link_libraries(squawk PRIVATE simpleCrypt)
target_link_libraries(squawk PRIVATE uuid)

# Build type

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

message("Build type: ${CMAKE_BUILD_TYPE}")

target_compile_options(squawk PRIVATE
  "-Wall;-Wextra"
  "$<$<CONFIG:DEBUG>:-g>"
  "$<$<CONFIG:RELEASE>:-O3>"
)

add_subdirectory(shared)

configure_file(resources/images/logo.svg squawk.svg COPYONLY)
execute_process(COMMAND convert -background none -size 48x48 squawk.svg squawk48.png WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
execute_process(COMMAND convert -background none -size 64x64 squawk.svg squawk64.png WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
execute_process(COMMAND convert -background none -size 128x128 squawk.svg squawk128.png WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
execute_process(COMMAND convert -background none -size 256x256 squawk.svg squawk256.png WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

configure_file(packaging/squawk.desktop squawk.desktop COPYONLY)

set(TS_FILES
    translations/squawk.ru.ts
)
qt5_add_translation(QM_FILES ${TS_FILES})
add_custom_target(translations ALL DEPENDS ${QM_FILES})

qt5_add_resources(RCC resources/resources.qrc)

target_sources(squawk PRIVATE ${RCC})

add_subdirectory(ui)
add_subdirectory(core)
add_subdirectory(plugins)

add_subdirectory(external/simpleCrypt)

add_dependencies(${CMAKE_PROJECT_NAME} translations)

# Install the executable
install(TARGETS squawk DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES ${QM_FILES} DESTINATION ${CMAKE_INSTALL_DATADIR}/squawk/l10n)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/squawk.svg DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/squawk48.png DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/48x48/apps RENAME squawk.png)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/squawk64.png DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/64x64/apps RENAME squawk.png)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/squawk128.png DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/128x128/apps RENAME squawk.png)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/squawk256.png DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/256x256/apps RENAME squawk.png)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/squawk.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
