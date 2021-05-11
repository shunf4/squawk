target_sources(squawk PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}/account.cpp
  ${CMAKE_CURRENT_LIST_DIR}/account.h
  ${CMAKE_CURRENT_LIST_DIR}/account.ui
  ${CMAKE_CURRENT_LIST_DIR}/accounts.cpp
  ${CMAKE_CURRENT_LIST_DIR}/accounts.h
  ${CMAKE_CURRENT_LIST_DIR}/accounts.ui
  ${CMAKE_CURRENT_LIST_DIR}/chat.cpp
  ${CMAKE_CURRENT_LIST_DIR}/chat.h
  ${CMAKE_CURRENT_LIST_DIR}/conversation.cpp
  ${CMAKE_CURRENT_LIST_DIR}/conversation.h
  ${CMAKE_CURRENT_LIST_DIR}/conversation.ui
  ${CMAKE_CURRENT_LIST_DIR}/joinconference.cpp
  ${CMAKE_CURRENT_LIST_DIR}/joinconference.h
  ${CMAKE_CURRENT_LIST_DIR}/joinconference.ui
  ${CMAKE_CURRENT_LIST_DIR}/newcontact.cpp
  ${CMAKE_CURRENT_LIST_DIR}/newcontact.h
  ${CMAKE_CURRENT_LIST_DIR}/newcontact.ui
  ${CMAKE_CURRENT_LIST_DIR}/room.cpp
  ${CMAKE_CURRENT_LIST_DIR}/room.h
  )

add_subdirectory(vcard)
