if (WITH_KIO)
  add_library(openFileManagerWindowJob SHARED openfilemanagerwindowjob.cpp)
  target_link_libraries(openFileManagerWindowJob PRIVATE KF5::KIOWidgets)
endif ()
