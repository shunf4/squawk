configure_file(squawk.desktop squawk.desktop COPYONLY)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/squawk.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)