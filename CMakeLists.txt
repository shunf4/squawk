cmake_minimum_required(VERSION 3.4)
project(squawk)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

include(GNUInstallDirs)
include_directories(.)

find_package(Qt5Widgets CONFIG REQUIRED)
find_package(Qt5QuickCompiler CONFIG REQUIRED)
find_package(Qt5LinguistTools)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_CXX_FLAGS_DEBUG "-g -Wall -Wextra")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
message("Build type: ${CMAKE_BUILD_TYPE}")


set(squawk_SRC
  main.cpp
  exception.cpp
  signalcatcher.cpp
  shared/global.cpp
  shared/utils.cpp
  shared/message.cpp
  shared/vcard.cpp
  shared/icons.cpp
)

set(squawk_HEAD
  exception.h
  signalcatcher.h
  shared.h
  shared/enums.h
  shared/message.h
  shared/global.h
  shared/utils.h
  shared/vcard.h
  shared/icons.h
)

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

option(SYSTEM_QXMPP "Use system qxmpp lib" ON) 
option(WITH_KWALLET "Build KWallet support module" ON) 

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

add_executable(squawk ${squawk_SRC} ${squawk_HEAD} ${RCC})
target_link_libraries(squawk Qt5::Widgets)

add_subdirectory(ui)
add_subdirectory(core)

add_subdirectory(external/simpleCrypt)

target_link_libraries(squawk squawkUI)
target_link_libraries(squawk squawkCORE)
target_link_libraries(squawk uuid)

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
