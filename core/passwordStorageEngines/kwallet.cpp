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

#include "kwallet.h"

Core::PSE::KWallet::OpenWallet Core::PSE::KWallet::openWallet = 0;
Core::PSE::KWallet::NetworkWallet Core::PSE::KWallet::networkWallet = 0;
Core::PSE::KWallet::DeleteWallet Core::PSE::KWallet::deleteWallet = 0;
Core::PSE::KWallet::ReadPassword Core::PSE::KWallet::readPassword = 0;
Core::PSE::KWallet::WritePassword Core::PSE::KWallet::writePassword = 0;
Core::PSE::KWallet::HasFolder Core::PSE::KWallet::hasFolder = 0;
Core::PSE::KWallet::CreateFolder Core::PSE::KWallet::createFolder = 0;
Core::PSE::KWallet::SetFolder Core::PSE::KWallet::setFolder = 0;

Core::PSE::KWallet::SupportState Core::PSE::KWallet::sState = Core::PSE::KWallet::initial;
QLibrary Core::PSE::KWallet::lib("kwalletWrapper");

Core::PSE::KWallet::KWallet():
    QObject(),
    cState(disconnected),
    everError(false),
    wallet(0),
    readRequest(),
    writeRequest()
{
    if (sState == initial) {
        lib.load();
        
        if (lib.isLoaded()) {
            openWallet = (OpenWallet) lib.resolve("openWallet");
            networkWallet = (NetworkWallet) lib.resolve("networkWallet");
            deleteWallet = (DeleteWallet) lib.resolve("deleteWallet");
            readPassword = (ReadPassword) lib.resolve("readPassword");
            writePassword = (WritePassword) lib.resolve("writePassword");
            hasFolder = (HasFolder) lib.resolve("hasFolder");
            createFolder = (CreateFolder) lib.resolve("createFolder");
            setFolder = (SetFolder) lib.resolve("setFolder");
            
            if (openWallet 
                && networkWallet
                && deleteWallet
                && readPassword
                && writePassword
            ) {
                sState = success;
            } else {
                sState = failure;
            }
        } else {
            sState = failure;
        }
    }
}

Core::PSE::KWallet::~KWallet()
{
    close();
}

void Core::PSE::KWallet::open()
{
    if (sState == success) {
        if (cState == disconnected) {
            QString name;
            networkWallet(name);
            wallet = openWallet(name, 0, ::KWallet::Wallet::Asynchronous);
            if (wallet) {
                cState = connecting;
                connect(wallet, SIGNAL(walletOpened(bool)), this, SLOT(onWalletOpened(bool)));
                connect(wallet, SIGNAL(walletClosed()), this, SLOT(onWalletClosed()));
            } else {
                everError = true;
                emit opened(false);
                rejectPending();
            }
        }
    }
}

Core::PSE::KWallet::ConnectionState Core::PSE::KWallet::connectionState()
{
    return cState;
}

void Core::PSE::KWallet::close()
{
    if (sState == success) {
        if (cState != disconnected) {
            deleteWallet(wallet);
            wallet = 0;
        }
        rejectPending();
    }
}

void Core::PSE::KWallet::onWalletClosed()
{
    cState = disconnected;
    deleteWallet(wallet);
    wallet = 0;
    emit closed();
    rejectPending();
}

void Core::PSE::KWallet::onWalletOpened(bool success)
{
    emit opened(success);
    if (success) {
        QString appName = QCoreApplication::applicationName();
        if (!hasFolder(wallet, appName)) {
            createFolder(wallet, appName);
        }
        setFolder(wallet, appName);
        cState = connected;
        readPending();
    } else {
        everError = true;
        cState = disconnected;
        deleteWallet(wallet);
        wallet = 0;
        rejectPending();
    }
}

Core::PSE::KWallet::SupportState Core::PSE::KWallet::supportState()
{
    return sState;
}

bool Core::PSE::KWallet::everHadError() const
{
    return everError;
}

void Core::PSE::KWallet::resetEverHadError()
{
    everError = false;
}

void Core::PSE::KWallet::requestReadPassword(const QString& login, bool askAgain)
{
    if (sState == success) {
        readRequest.insert(login);
        readSwitch(askAgain);
    }
}

void Core::PSE::KWallet::requestWritePassword(const QString& login, const QString& password, bool askAgain)
{
    if (sState == success) {
        std::map<QString, QString>::iterator itr = writeRequest.find(login);
        if (itr == writeRequest.end()) {
            writeRequest.insert(std::make_pair(login, password));
        } else {
            itr->second = password;
        }
        readSwitch(askAgain);
    }
}

void Core::PSE::KWallet::readSwitch(bool askAgain)
{
    switch (cState) {
        case connected:
            readPending();
            break;
        case connecting:
            break;
        case disconnected:
            if (!everError || askAgain) {
                open();
            }
            break;
    }
}

void Core::PSE::KWallet::rejectPending()
{
    writeRequest.clear();
    std::set<QString>::const_iterator i = readRequest.begin();
    while (i != readRequest.end()) {
        emit rejectPassword(*i);
        readRequest.erase(i);
        i = readRequest.begin();
    }
}

void Core::PSE::KWallet::readPending()
{
    std::map<QString, QString>::const_iterator itr = writeRequest.begin();
    while (itr != writeRequest.end()) {
        int result = writePassword(wallet, itr->first, itr->second);
        if (result == 0) {
            qDebug() << "Successfully saved password for user" << itr->first;
        } else {
            qDebug() << "Error writing password for user" << itr->first << ":" << result;
        }
        writeRequest.erase(itr);
        itr = writeRequest.begin();
    }
    
    std::set<QString>::const_iterator i = readRequest.begin();
    while (i != readRequest.end()) {
        QString password;
        int result = readPassword(wallet, *i, password);
        if (result == 0 && password.size() > 0) {       //even though it's written that the error is supposed to be returned in case there were no password
            emit responsePassword(*i, password);        //it doesn't do so. I assume empty password as a lack of password in KWallet
        } else {
            emit rejectPassword(*i);
        }
        readRequest.erase(i);
        i = readRequest.begin();
    }
}

