target_sources(squawk PRIVATE resources.qrc)

configure_file(images/logo.svg squawk.svg COPYONLY)

execute_process(COMMAND convert -background none -size 48x48 squawk.svg squawk48.png WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
execute_process(COMMAND convert -background none -size 64x64 squawk.svg squawk64.png WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
execute_process(COMMAND convert -background none -size 128x128 squawk.svg squawk128.png WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
execute_process(COMMAND convert -background none -size 256x256 squawk.svg squawk256.png WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/squawk.svg DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/squawk48.png DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/48x48/apps RENAME squawk.png)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/squawk64.png DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/64x64/apps RENAME squawk.png)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/squawk128.png DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/128x128/apps RENAME squawk.png)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/squawk256.png DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/256x256/apps RENAME squawk.png)
