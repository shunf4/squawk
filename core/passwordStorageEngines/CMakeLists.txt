cmake_minimum_required(VERSION 3.0)
project(pse)


if (WITH_KWALLET) 
    set(CMAKE_AUTOMOC ON)

    find_package(Qt5Core CONFIG REQUIRED)
    find_package(Qt5Gui CONFIG REQUIRED)

    get_target_property(KWALLET_INTERFACE_INCLUDE_DIRECTORIES KF5::Wallet INTERFACE_INCLUDE_DIRECTORIES)
    get_target_property(Qt5GUI_INTERFACE_INCLUDE_DIRECTORIES Qt5::Gui INTERFACE_INCLUDE_DIRECTORIES)

    set(kwalletPSE_SRC
        kwallet.cpp
    )
    
    add_library(kwalletPSE ${kwalletPSE_SRC})
    
    target_include_directories(kwalletPSE PUBLIC ${KWALLET_INTERFACE_INCLUDE_DIRECTORIES})
    target_include_directories(kwalletPSE PUBLIC ${Qt5GUI_INTERFACE_INCLUDE_DIRECTORIES})

    target_link_libraries(kwalletPSE Qt5::Core)

    set(kwalletW_SRC
        wrappers/kwallet.cpp
    )

    add_library(kwalletWrapper SHARED ${kwalletW_SRC})

    target_include_directories(kwalletWrapper PUBLIC ${KWALLET_INTERFACE_INCLUDE_DIRECTORIES})
    target_include_directories(kwalletWrapper PUBLIC ${Qt5GUI_INTERFACE_INCLUDE_DIRECTORIES})

    target_link_libraries(kwalletWrapper KF5::Wallet)
    target_link_libraries(kwalletWrapper Qt5::Core)

    install(TARGETS kwalletWrapper DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()




