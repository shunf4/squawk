cmake_minimum_required(VERSION 3.3)
project(plugins)

if (WITH_KIO) 
    set(CMAKE_AUTOMOC ON)
    
    find_package(Qt5Core CONFIG REQUIRED)

    set(openFileManagerWindowJob_SRC
        openfilemanagerwindowjob.cpp
    )

    add_library(openFileManagerWindowJob SHARED ${openFileManagerWindowJob_SRC})
    
    get_target_property(Qt5CORE_INTERFACE_INCLUDE_DIRECTORIES Qt5::Core INTERFACE_INCLUDE_DIRECTORIES)
    get_target_property(KIO_WIDGETS_INTERFACE_INCLUDE_DIRECTORIES KF5::KIOWidgets INTERFACE_INCLUDE_DIRECTORIES)
    get_target_property(CORE_ADDONS_INTERFACE_INCLUDE_DIRECTORIES KF5::CoreAddons INTERFACE_INCLUDE_DIRECTORIES)
    target_include_directories(openFileManagerWindowJob PUBLIC ${KIO_WIDGETS_INTERFACE_INCLUDE_DIRECTORIES})
    target_include_directories(openFileManagerWindowJob PUBLIC ${CORE_ADDONS_INTERFACE_INCLUDE_DIRECTORIES})
    target_include_directories(openFileManagerWindowJob PUBLIC ${Qt5CORE_INTERFACE_INCLUDE_DIRECTORIES})
    
    target_link_libraries(openFileManagerWindowJob KF5::KIOWidgets)
    target_link_libraries(openFileManagerWindowJob Qt5::Core)

    install(TARGETS openFileManagerWindowJob DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()
