if (WITH_KWALLET)
  target_sources(squawk PRIVATE
    wrappers/kwallet.cpp
    kwallet.cpp
    kwallet.h
    )

  target_link_libraries(squawk PUBLIC KF5::Wallet)
endif ()
