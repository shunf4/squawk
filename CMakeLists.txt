cmake_minimum_required(VERSION 3.0)
project(squawk)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 14)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

include(GNUInstallDirs)
include_directories(.)

find_package(Qt5Widgets CONFIG REQUIRED)
find_package(Qt5LinguistTools)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

if(CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS_DEBUG "-g -Wall -Wextra")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3")
endif(CMAKE_COMPILER_IS_GNUCXX)

message("Build type: ${CMAKE_BUILD_TYPE}")


set(squawk_SRC
  main.cpp
  exception.cpp
  shared/global.cpp
  shared/utils.cpp
  shared/message.cpp
  shared/vcard.cpp
  shared/icons.cpp
)

if (WIN32)
    list(APPEND squawk_SRC signalcatcher_win32.cpp)
else (WIN32)
    list(APPEND squawk_SRC signalcatcher.cpp)
endif (WIN32)


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
if (WIN32)
    set(CONVERT_BIN magick convert)
else(WIN32)
    set(CONVERT_BIN convert)
endif(WIN32)

execute_process(COMMAND ${CONVERT_BIN} -background none -size 48x48 squawk.svg squawk48.png WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
execute_process(COMMAND ${CONVERT_BIN} -background none -size 64x64 squawk.svg squawk64.png WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
execute_process(COMMAND ${CONVERT_BIN} -background none -size 128x128 squawk.svg squawk128.png WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
execute_process(COMMAND ${CONVERT_BIN} -background none -size 256x256 squawk.svg squawk256.png WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

if (WIN32)
    execute_process(COMMAND ${CONVERT_BIN} squawk48.png squawk64.png squawk256.png squawk.ico WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    set(SQUAWK_WIN_RC "${CMAKE_CURRENT_BINARY_DIR}/squawk.rc")
endif(WIN32)

if (APPLE)
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/icns.iconset")
    execute_process(COMMAND convert -background none -size 16x16 squawk.svg icns.iconset/icon_16x16.png WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    execute_process(COMMAND convert -background none -resize !32x32 squawk.svg "icns.iconset/icon_16x16@2x.png" WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    execute_process(COMMAND convert -background none -resize !32x32 squawk.svg "icns.iconset/icon_32x32.png" WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    execute_process(COMMAND convert -background none -resize !64x64 squawk.svg "icns.iconset/icon_32x32@2x.png" WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    execute_process(COMMAND convert -background none -resize !128x128 squawk.svg "icns.iconset/icon_128x128.png" WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    execute_process(COMMAND convert -background none -resize !256x256 squawk.svg "icns.iconset/icon_128x128@2x.png" WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    execute_process(COMMAND convert -background none -resize !256x256 squawk.svg "icns.iconset/icon_256x256.png" WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    execute_process(COMMAND convert -background none -resize !512x512 squawk.svg "icns.iconset/icon_256x256@2x.png" WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    execute_process(COMMAND convert -background none -resize !512x512 squawk.svg "icns.iconset/icon_512x512.png" WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    execute_process(COMMAND convert -background none -resize !1024x1024 squawk.svg "icns.iconset/icon_512x512@2x.png" WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    execute_process(COMMAND iconutil -c icns "icns.iconset" -o "squawk.icns" WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    set(MACOSX_BUNDLE_ICON_FILE squawk.icns)
    set(APP_ICON_MACOSX ${CMAKE_CURRENT_BINARY_DIR}/squawk.icns)
    set_source_files_properties(${APP_ICON_MACOSX} PROPERTIES
               MACOSX_PACKAGE_LOCATION "Resources")
endif (APPLE)

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

set(WIN32_FLAG "")
if (WIN32)
    if (CMAKE_BUILD_TYPE STREQUAL "Release")
        set(WIN32_FLAG WIN32)
    endif()
endif(WIN32)
add_executable(squawk ${WIN32_FLAG} ${squawk_SRC} ${squawk_HEAD} ${RCC} ${SQUAWK_WIN_RC} ${APP_ICON_MACOSX})
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
