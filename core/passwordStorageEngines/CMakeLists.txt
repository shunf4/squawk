if (WITH_KWALLET)
  target_sources(squawk PRIVATE
    kwallet.cpp
    kwallet.h
    )

  add_subdirectory(wrappers)
  target_include_directories(squawk PRIVATE $<TARGET_PROPERTY:KF5::Wallet,INTERFACE_INCLUDE_DIRECTORIES>)
endif ()
