#include <KF5/KWallet/KWallet>

extern "C" KWallet::Wallet* openWallet(const QString &name, WId w, KWallet::Wallet::OpenType ot = KWallet::Wallet::Synchronous) {
    return KWallet::Wallet::openWallet(name, w, ot);
}

extern "C" void deleteWallet(KWallet::Wallet* w) {
    w->deleteLater();
}

extern "C" void networkWallet(QString& str) {
    str = KWallet::Wallet::NetworkWallet();
}

extern "C" int readPassword(KWallet::Wallet* w, const QString &key, QString &value) {
    return w->readPassword(key, value);
}

extern "C" int writePassword(KWallet::Wallet* w, const QString &key, const QString &value) {
    return w->writePassword(key, value);
}

extern "C" bool hasFolder(KWallet::Wallet* w, const QString &f) {
    return w->hasFolder(f);
}

extern "C" bool createFolder(KWallet::Wallet* w, const QString &f) {
    return w->createFolder(f);
}

extern "C" bool setFolder(KWallet::Wallet* w, const QString &f) {
    return w->setFolder(f);
}
