find_package(Qt5LinguistTools)

set(TS_FILES squawk.ru.ts)
qt5_add_translation(QM_FILES ${TS_FILES})
add_custom_target(translations ALL DEPENDS ${QM_FILES})
install(FILES ${QM_FILES} DESTINATION ${CMAKE_INSTALL_DATADIR}/squawk/l10n)

add_dependencies(${CMAKE_PROJECT_NAME} translations)