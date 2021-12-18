target_sources(squawk PRIVATE
  account.cpp
  account.h
  account.ui
  accounts.cpp
  accounts.h
  accounts.ui
  chat.cpp
  chat.h
  conversation.cpp
  conversation.h
  conversation.ui
  joinconference.cpp
  joinconference.h
  joinconference.ui
  newcontact.cpp
  newcontact.h
  newcontact.ui
  room.cpp
  room.h
  messagetextedit.cpp
  messagetextedit.h
  )

add_subdirectory(vcard)
add_subdirectory(messageline)
