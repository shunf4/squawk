cmake_minimum_required(VERSION 3.0)
project(squawk)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5Widgets CONFIG REQUIRED)
find_package(Qt5LinguistTools)

set(squawk_SRC
  main.cpp
  global.cpp
  exception.cpp
  signalcatcher.cpp
)

set(TS_FILES 
    translations/squawk.ru.ts
)
qt5_add_translation(QM_FILES ${TS_FILES})
add_custom_target(translations ALL DEPENDS ${QM_FILES})

qt5_add_resources(RCC resources/resources.qrc)

add_executable(squawk ${squawk_SRC} ${RCC})
target_link_libraries(squawk Qt5::Widgets)

add_subdirectory(ui)
add_subdirectory(core)
add_subdirectory(external/qxmpp)

target_link_libraries(squawk squawkUI)
target_link_libraries(squawk squawkCORE)
target_link_libraries(squawk uuid)

add_dependencies(${CMAKE_PROJECT_NAME} translations)

# Install the executable
install(TARGETS squawk DESTINATION bin)
install(FILES ${QM_FILES} DESTINATION share/l10n)
