add_library(kwalletWrapper SHARED kwallet.cpp)
target_link_libraries(kwalletWrapper PRIVATE KF5::Wallet)
