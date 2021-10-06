target_sources(squawk PRIVATE resources.qrc)

configure_file(images/logo.svg squawk.svg COPYONLY)
configure_file(squawk.rc squawk.rc COPYONLY)

if(WIN32)
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
    set(SQUAWK_WIN_RC "${SQUAWK_WIN_RC}" PARENT_SCOPE)
    target_sources(squawk PRIVATE ${SQUAWK_WIN_RC})
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
    set(MACOSX_BUNDLE_ICON_FILE ${MACOSX_BUNDLE_ICON_FILE} PARENT_SCOPE)
    set(APP_ICON_MACOSX ${CMAKE_CURRENT_BINARY_DIR}/squawk.icns)
    set(APP_ICON_MACOSX ${APP_ICON_MACOSX} PARENT_SCOPE)
    target_sources(squawk PRIVATE ${APP_ICON_MACOSX})
    set_source_files_properties(${APP_ICON_MACOSX} TARGET_DIRECTORY squawk PROPERTIES
               MACOSX_PACKAGE_LOCATION "Resources")
    if (CMAKE_BUILD_TYPE STREQUAL "Release")
      if (APPLE)
        set_target_properties(squawk PROPERTIES
          MACOSX_BUNDLE_EXECUTABLE_NAME "Squawk"
          MACOSX_BUNDLE_ICON_FILE "${MACOSX_BUNDLE_ICON_FILE}" # TODO
          MACOSX_BUNDLE_BUNDLE_NAME "Squawk"
          MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/cmake/MacOSXBundleInfo.plist.in)
      endif(APPLE)
    endif()
endif (APPLE)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/squawk.svg DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/squawk48.png DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/48x48/apps RENAME squawk.png)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/squawk64.png DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/64x64/apps RENAME squawk.png)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/squawk128.png DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/128x128/apps RENAME squawk.png)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/squawk256.png DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/256x256/apps RENAME squawk.png)

