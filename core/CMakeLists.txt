set(SIGNALCATCHER_SOURCE signalcatcher.cpp)
if(WIN32)
    set(SIGNALCATCHER_SOURCE signalcatcher_win32.cpp)
endif(WIN32)

target_sources(squawk PRIVATE
  account.cpp
  account.h
  adapterFuctions.cpp
  archive.cpp
  archive.h
  conference.cpp
  conference.h
  contact.cpp
  contact.h
  main.cpp
  networkaccess.cpp
  networkaccess.h
  rosteritem.cpp
  rosteritem.h
  ${SIGNALCATCHER_SOURCE}
  signalcatcher.h
  squawk.cpp
  squawk.h
  storage.cpp
  storage.h
  urlstorage.cpp
  urlstorage.h
  )

add_subdirectory(handlers)
add_subdirectory(passwordStorageEngines)
