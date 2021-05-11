target_sources(squawk PRIVATE
  wrappers/kwallet.cpp
  kwallet.cpp
  kwallet.h
  )

if (WITH_KWALLET)
#    get_target_property(KWALLET_INTERFACE_INCLUDE_DIRECTORIES KF5::Wallet INTERFACE_INCLUDE_DIRECTORIES)
#    get_target_property(Qt5GUI_INTERFACE_INCLUDE_DIRECTORIES Qt5::Gui INTERFACE_INCLUDE_DIRECTORIES)
#
#    target_include_directories(squawk PRIVATE ${KWALLET_INTERFACE_INCLUDE_DIRECTORIES})
#    target_include_directories(squawk PRIVATE ${Qt5GUI_INTERFACE_INCLUDE_DIRECTORIES})

    target_link_libraries(squawk PUBLIC Qt5::Core Qt5::Gui KF5::Wallet)

#    target_include_directories(kwalletWrapper PUBLIC ${KWALLET_INTERFACE_INCLUDE_DIRECTORIES})
#    target_include_directories(kwalletWrapper PUBLIC ${Qt5GUI_INTERFACE_INCLUDE_DIRECTORIES})

#    target_link_libraries(kwalletWrapper KF5::Wallet)
#    target_link_libraries(kwalletWrapper Qt5::Core)
endif()
