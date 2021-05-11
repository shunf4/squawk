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
  signalcatcher.cpp
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
