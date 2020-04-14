/*
 * Squawk messenger. 
 * Copyright (C) 2019  Yury Gubich <blue@macaw.me>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef CORE_PSE_KWALLET_H
#define CORE_PSE_KWALLET_H

#include <QObject>
#include <QLibrary>
#include <QDebug>
#include <QCoreApplication>

#include <map>
#include <set>

#include <KF5/KWallet/KWallet>

namespace Core {
namespace PSE {

/**
 * @todo write docs
 */
class KWallet : public QObject
{
    Q_OBJECT
public:
    enum SupportState {
        initial,
        success,
        failure
    };
    enum ConnectionState {
        disconnected,
        connecting,
        connected
    };
    
    KWallet();
    ~KWallet();
    
    static SupportState supportState();
    void open();
    void close();
    ConnectionState connectionState();
    bool everHadError() const;
    void resetEverHadError();
    void requestReadPassword(const QString& login, bool askAgain = false);
    void requestWritePassword(const QString& login, const QString& password, bool askAgain = false);
    
signals:
    void opened(bool success);
    void closed();
    void responsePassword(const QString& login, const QString& password);
    void rejectPassword(const QString& login);
    
private slots:
    void onWalletOpened(bool success);
    void onWalletClosed();
    
private:
    void readSwitch(bool askAgain);
    void readPending();
    void rejectPending();
    
private:
    typedef ::KWallet::Wallet* (*OpenWallet)(const QString &, WId, ::KWallet::Wallet::OpenType);
    typedef void (*NetworkWallet)(QString&);
    typedef void (*DeleteWallet)(::KWallet::Wallet*);
    typedef int (*ReadPassword)(::KWallet::Wallet*, const QString&, QString&);
    typedef int (*WritePassword)(::KWallet::Wallet*, const QString&, const QString&);
    typedef bool (*HasFolder)(::KWallet::Wallet* w, const QString &f);
    typedef bool (*CreateFolder)(::KWallet::Wallet* w, const QString &f);
    typedef bool (*SetFolder)(::KWallet::Wallet* w, const QString &f);
    
    static OpenWallet openWallet;
    static NetworkWallet networkWallet;
    static DeleteWallet deleteWallet;
    static ReadPassword readPassword;
    static WritePassword writePassword;
    static HasFolder hasFolder;
    static CreateFolder createFolder;
    static SetFolder setFolder;
    
    static SupportState sState;
    static QLibrary lib;
    
    ConnectionState cState;
    bool everError;
    ::KWallet::Wallet* wallet;
    
    std::set<QString> readRequest;
    std::map<QString, QString> writeRequest;
};

}}

#endif // CORE_PSE_KWALLET_H
