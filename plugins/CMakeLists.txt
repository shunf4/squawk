if (WITH_KIO)
  target_sources(squawk PRIVATE openfilemanagerwindowjob.cpp)
  target_link_libraries(squawk PRIVATE KF5::KIOWidgets)
endif ()
