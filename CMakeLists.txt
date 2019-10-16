cmake_minimum_required(VERSION 3.0)
project(squawk)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 14)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

include(GNUInstallDirs)

find_package(Qt5Widgets CONFIG REQUIRED)
find_package(Qt5LinguistTools)

set(squawk_SRC
  main.cpp
  global.cpp
  exception.cpp
  signalcatcher.cpp
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

add_executable(squawk ${squawk_SRC} ${RCC})
target_link_libraries(squawk Qt5::Widgets)

option(SYSTEM_QXMPP "Use system qxmpp lib" ON) 

if(NOT SYSTEM_QXMPP)
    add_subdirectory(external/qxmpp)
endif()

add_subdirectory(ui)
add_subdirectory(core)

target_link_libraries(squawk squawkUI)
target_link_libraries(squawk squawkCORE)
target_link_libraries(squawk uuid)

add_dependencies(${CMAKE_PROJECT_NAME} translations)

# Install the executable
install(TARGETS squawk DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES ${QM_FILES} DESTINATION ${CMAKE_INSTALL_DATADIR}/squawk/l10n)
install(FILES squawk.svg DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)
install(FILES squawk48.png DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/48x48/apps RENAME squawk.png)
install(FILES squawk64.png DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/64x64/apps RENAME squawk.png)
install(FILES squawk128.png DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/128x128/apps RENAME squawk.png)
install(FILES squawk256.png DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/256x256/apps RENAME squawk.png)
install(FILES squawk.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
