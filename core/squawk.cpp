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

#include "squawk.h"
#include <QDebug>
#include <QSettings>
#include <QDir>
#include <QStandardPaths>

Core::Squawk::Squawk(QObject* parent):
    QObject(parent),
    accounts(),
    amap(),
    network(),
    waitingForAccounts(0)
#ifdef WITH_KWALLET
    ,kwallet()
#endif
{
    connect(&network, &NetworkAccess::loadFileProgress, this, &Squawk::fileProgress);
    connect(&network, &NetworkAccess::loadFileError, this, &Squawk::fileError);
    connect(&network, &NetworkAccess::downloadFileComplete, this, &Squawk::fileDownloadComplete);
    connect(&network, &NetworkAccess::uploadFileComplete, this, &Squawk::fileUploadComplete);
    
#ifdef WITH_KWALLET
    if (kwallet.supportState() == PSE::KWallet::success) {
        connect(&kwallet, &PSE::KWallet::opened, this, &Squawk::onWalletOpened);
        connect(&kwallet, &PSE::KWallet::rejectPassword, this, &Squawk::onWalletRejectPassword);
        connect(&kwallet, &PSE::KWallet::responsePassword, this, &Squawk::onWalletResponsePassword);
        
        Shared::Global::setSupported("KWallet", true);
    }
#endif
}

Core::Squawk::~Squawk()
{
    Accounts::const_iterator itr = accounts.begin(); 
    Accounts::const_iterator end = accounts.end();
    for (; itr != end; ++itr) {
        delete (*itr);
    }
}

void Core::Squawk::onWalletOpened(bool success)
{
    qDebug() << "KWallet opened: " << success;
}

void Core::Squawk::stop()
{
    qDebug("Stopping squawk core..");
    network.stop();
    QSettings settings;
    settings.beginGroup("core");
    settings.beginWriteArray("accounts");
    SimpleCrypt crypto(passwordHash);
    for (std::deque<Account*>::size_type i = 0; i < accounts.size(); ++i) {
        settings.setArrayIndex(i);
        Account* acc = accounts[i];
        
        Shared::AccountPassword ap = acc->getPasswordType();
        QString password;
        
        switch (ap) {
            case Shared::AccountPassword::plain:
                password = acc->getPassword();
                break;
            case Shared::AccountPassword::jammed:
                password = crypto.encryptToString(acc->getPassword());
                break;
            default:
                break;
        }
        
        settings.setValue("name", acc->getName());
        settings.setValue("server", acc->getServer());
        settings.setValue("login", acc->getLogin());
        settings.setValue("password", password);
        settings.setValue("resource", acc->getResource());
        settings.setValue("passwordType", static_cast<int>(ap));
    }
    settings.endArray();
    settings.endGroup();
    
    settings.sync();
    
    emit quit();
}

void Core::Squawk::start()
{
    qDebug("Starting squawk core..");
    
    readSettings();
    network.start();
}

void Core::Squawk::newAccountRequest(const QMap<QString, QVariant>& map)
{
    QString name = map.value("name").toString();
    QString login = map.value("login").toString();
    QString server = map.value("server").toString();
    QString password = map.value("password").toString();
    QString resource = map.value("resource").toString();
    
    addAccount(login, server, password, name, resource, Shared::AccountPassword::plain);
}

void Core::Squawk::addAccount(
    const QString& login, 
    const QString& server, 
    const QString& password, 
    const QString& name, 
    const QString& resource,                          
    Shared::AccountPassword passwordType
)
{
    QSettings settings;
    
    Account* acc = new Account(login, server, password, name, &network);
    acc->setResource(resource);
    acc->setPasswordType(passwordType);
    accounts.push_back(acc);
    amap.insert(std::make_pair(name, acc));
    
    connect(acc, &Account::connectionStateChanged, this, &Squawk::onAccountConnectionStateChanged);
    connect(acc, &Account::changed, this, &Squawk::onAccountChanged);
    connect(acc, &Account::error, this, &Squawk::onAccountError);
    connect(acc, &Account::availabilityChanged, this, &Squawk::onAccountAvailabilityChanged);
    connect(acc, &Account::addContact, this, &Squawk::onAccountAddContact);
    connect(acc, &Account::addGroup, this, &Squawk::onAccountAddGroup);
    connect(acc, &Account::removeGroup, this, &Squawk::onAccountRemoveGroup);
    connect(acc, qOverload<const QString&, const QString&>(&Account::removeContact), 
            this, qOverload<const QString&, const QString&>(&Squawk::onAccountRemoveContact));
    connect(acc, qOverload<const QString&>(&Account::removeContact), 
            this, qOverload<const QString&>(&Squawk::onAccountRemoveContact));
    connect(acc, &Account::changeContact, this, &Squawk::onAccountChangeContact);
    connect(acc, &Account::addPresence, this, &Squawk::onAccountAddPresence);
    connect(acc, &Account::removePresence, this, &Squawk::onAccountRemovePresence);
    connect(acc, &Account::message, this, &Squawk::onAccountMessage);
    connect(acc, &Account::changeMessage, this, &Squawk::onAccountChangeMessage);
    connect(acc, &Account::responseArchive, this, &Squawk::onAccountResponseArchive);

    connect(acc, &Account::addRoom, this, &Squawk::onAccountAddRoom);
    connect(acc, &Account::changeRoom, this, &Squawk::onAccountChangeRoom);
    connect(acc, &Account::removeRoom, this, &Squawk::onAccountRemoveRoom);
    
    connect(acc, &Account::addRoomParticipant, this, &Squawk::onAccountAddRoomPresence);
    connect(acc, &Account::changeRoomParticipant, this, &Squawk::onAccountChangeRoomPresence);
    connect(acc, &Account::removeRoomParticipant, this, &Squawk::onAccountRemoveRoomPresence);
    
    connect(acc, &Account::receivedVCard, this, &Squawk::responseVCard);
    
    connect(acc, &Account::uploadFileError, this, &Squawk::onAccountUploadFileError);
    
    QMap<QString, QVariant> map = {
        {"login", login},
        {"server", server},
        {"name", name},
        {"password", password},
        {"resource", resource},
        {"state", QVariant::fromValue(Shared::ConnectionState::disconnected)},
        {"offline", QVariant::fromValue(Shared::Availability::offline)},
        {"error", ""},
        {"avatarPath", acc->getAvatarPath()},
        {"passwordType", QVariant::fromValue(passwordType)}
    };
    
    emit newAccount(map);
}

void Core::Squawk::changeState(Shared::Availability p_state)
{
    if (state != p_state) {
        state = p_state;
    }
    
    for (std::deque<Account*>::iterator itr = accounts.begin(), end = accounts.end(); itr != end; ++itr) {
        (*itr)->setAvailability(state);
    }
}

void Core::Squawk::connectAccount(const QString& account)
{
    AccountsMap::const_iterator itr = amap.find(account);
    if (itr == amap.end()) {
        qDebug("An attempt to connect non existing account, skipping");
        return;
    }
    itr->second->connect();
}

void Core::Squawk::disconnectAccount(const QString& account)
{
    AccountsMap::const_iterator itr = amap.find(account);
    if (itr == amap.end()) {
        qDebug("An attempt to connect non existing account, skipping");
        return;
    }
    
    itr->second->disconnect();
}

void Core::Squawk::onAccountConnectionStateChanged(Shared::ConnectionState p_state)
{
    Account* acc = static_cast<Account*>(sender());
    emit changeAccount(acc->getName(), {{"state", QVariant::fromValue(p_state)}});
    
#ifdef WITH_KWALLET
    if (p_state == Shared::ConnectionState::connected) {
        if (acc->getPasswordType() == Shared::AccountPassword::kwallet && kwallet.supportState() == PSE::KWallet::success) {
            kwallet.requestWritePassword(acc->getName(), acc->getPassword(), true);
        }
    }
#endif
    
    Accounts::const_iterator itr = accounts.begin();
    bool es = true;
    bool ea = true;
    Shared::ConnectionState cs = (*itr)->getState();
    Shared::Availability av = (*itr)->getAvailability();
    itr++;
    for (Accounts::const_iterator end = accounts.end(); itr != end; itr++) {
        Account* item = *itr;
        if (item->getState() != cs) {
            es = false;
        }
        if (item->getAvailability() != av) {
            ea = false;
        }
    }
    
    if (es) {
        if (cs == Shared::ConnectionState::disconnected) {
            state = Shared::Availability::offline;
            emit stateChanged(state);
        } else if (ea) {
            state = av;
            emit stateChanged(state);
        }
    }
    
}

void Core::Squawk::onAccountAddContact(const QString& jid, const QString& group, const QMap<QString, QVariant>& data)
{
    Account* acc = static_cast<Account*>(sender());
    emit addContact(acc->getName(), jid, group, data);
}

void Core::Squawk::onAccountAddGroup(const QString& name)
{
    Account* acc = static_cast<Account*>(sender());
    emit addGroup(acc->getName(), name);
}

void Core::Squawk::onAccountRemoveGroup(const QString& name)
{
    Account* acc = static_cast<Account*>(sender());
    emit removeGroup(acc->getName(), name);
}

void Core::Squawk::onAccountChangeContact(const QString& jid, const QMap<QString, QVariant>& data)
{
    Account* acc = static_cast<Account*>(sender());
    emit changeContact(acc->getName(), jid, data);
}

void Core::Squawk::onAccountRemoveContact(const QString& jid)
{
    Account* acc = static_cast<Account*>(sender());
    emit removeContact(acc->getName(), jid);
}

void Core::Squawk::onAccountRemoveContact(const QString& jid, const QString& group)
{
    Account* acc = static_cast<Account*>(sender());
    emit removeContact(acc->getName(), jid, group);
}

void Core::Squawk::onAccountAddPresence(const QString& jid, const QString& name, const QMap<QString, QVariant>& data)
{
    Account* acc = static_cast<Account*>(sender());
    emit addPresence(acc->getName(), jid, name, data);
}

void Core::Squawk::onAccountRemovePresence(const QString& jid, const QString& name)
{
    Account* acc = static_cast<Account*>(sender());
    emit removePresence(acc->getName(), jid, name);
}

void Core::Squawk::onAccountAvailabilityChanged(Shared::Availability state)
{
    Account* acc = static_cast<Account*>(sender());
    emit changeAccount(acc->getName(), {{"availability", QVariant::fromValue(state)}});
}

void Core::Squawk::onAccountChanged(const QMap<QString, QVariant>& data)
{    
    Account* acc = static_cast<Account*>(sender());
    emit changeAccount(acc->getName(), data);
}

void Core::Squawk::onAccountMessage(const Shared::Message& data)
{
    Account* acc = static_cast<Account*>(sender());
    emit accountMessage(acc->getName(), data);
}

void Core::Squawk::sendMessage(const QString& account, const Shared::Message& data)
{
    AccountsMap::const_iterator itr = amap.find(account);
    if (itr == amap.end()) {
        qDebug("An attempt to send a message with non existing account, skipping");
        return;
    }
    
    itr->second->sendMessage(data);
}

void Core::Squawk::requestArchive(const QString& account, const QString& jid, int count, const QString& before)
{
    AccountsMap::const_iterator itr = amap.find(account);
    if (itr == amap.end()) {
        qDebug("An attempt to request an archive of non existing account, skipping");
        return;
    }
    itr->second->requestArchive(jid, count, before);
}

void Core::Squawk::onAccountResponseArchive(const QString& jid, const std::list<Shared::Message>& list, bool last)
{
    Account* acc = static_cast<Account*>(sender());
    emit responseArchive(acc->getName(), jid, list, last);
}

void Core::Squawk::modifyAccountRequest(const QString& name, const QMap<QString, QVariant>& map)
{
    AccountsMap::const_iterator itr = amap.find(name);
    if (itr == amap.end()) {
        qDebug("An attempt to modify non existing account, skipping");
        return;
    }
    
    Core::Account* acc = itr->second;
    Shared::ConnectionState st = acc->getState();
    QMap<QString, QVariant>::const_iterator mItr;
    bool needToReconnect = false;
    
    mItr = map.find("login");
    if (mItr != map.end()) {
        needToReconnect = acc->getLogin() != mItr->toString();
    }
    
    if (!needToReconnect) {
        mItr = map.find("password");
        if (mItr != map.end()) {
            needToReconnect = acc->getPassword() != mItr->toString();
        }
    }
    if (!needToReconnect) {
        mItr = map.find("server");
        if (mItr != map.end()) {
            needToReconnect = acc->getServer() != mItr->toString();
        }
    }
    if (!needToReconnect) {
        mItr = map.find("resource");
        if (mItr != map.end()) {
            needToReconnect = acc->getResource() != mItr->toString();
        }
    }
    
    if (needToReconnect && st != Shared::ConnectionState::disconnected) {
        acc->reconnect();
    }
    
    mItr = map.find("login");
    if (mItr != map.end()) {
        acc->setLogin(mItr->toString());
    }
    
    mItr = map.find("password");
    if (mItr != map.end()) {
        acc->setPassword(mItr->toString());
    }
    
    mItr = map.find("resource");
    if (mItr != map.end()) {
        acc->setResource(mItr->toString());
    }
    
    mItr = map.find("server");
    if (mItr != map.end()) {
        acc->setServer(mItr->toString());
    }
    
    mItr = map.find("passwordType");
    if (mItr != map.end()) {
        acc->setPasswordType(Shared::Global::fromInt<Shared::AccountPassword>(mItr->toInt()));
    }
    
#ifdef WITH_KWALLET
    if (acc->getPasswordType() == Shared::AccountPassword::kwallet 
        && kwallet.supportState() == PSE::KWallet::success 
        && !needToReconnect
    ) {
        kwallet.requestWritePassword(acc->getName(), acc->getPassword(), true);
    }
#endif
    
    emit changeAccount(name, map);
}

void Core::Squawk::onAccountError(const QString& text)
{
    Account* acc = static_cast<Account*>(sender());
    emit changeAccount(acc->getName(), {{"error", text}});
}

void Core::Squawk::removeAccountRequest(const QString& name)
{
    AccountsMap::const_iterator itr = amap.find(name);
    if (itr == amap.end()) {
        qDebug() << "An attempt to remove non existing account " << name << " from core, skipping";
        return;
    }
    
    Account* acc = itr->second;
    if (acc->getState() != Shared::ConnectionState::disconnected) {
        acc->disconnect();
    }
    
    for (Accounts::const_iterator aItr = accounts.begin(); aItr != accounts.end(); ++aItr) {
        if (*aItr == acc) {
            accounts.erase(aItr);
            break;
        }
    }
    
    amap.erase(itr);
    
    QString path(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
    path += "/" + name;
    QDir dir(path);
    dir.removeRecursively();
    
    emit removeAccount(name);
    acc->deleteLater();
}

void Core::Squawk::subscribeContact(const QString& account, const QString& jid, const QString& reason)
{
    AccountsMap::const_iterator itr = amap.find(account);
    if (itr == amap.end()) {
        qDebug("An attempt to subscribe to the contact with non existing account, skipping");
        return;
    }
    
    itr->second->subscribeToContact(jid, reason);
}

void Core::Squawk::unsubscribeContact(const QString& account, const QString& jid, const QString& reason)
{
    AccountsMap::const_iterator itr = amap.find(account);
    if (itr == amap.end()) {
        qDebug("An attempt to subscribe to the contact with non existing account, skipping");
        return;
    }
    
    itr->second->unsubscribeFromContact(jid, reason);
}

void Core::Squawk::removeContactRequest(const QString& account, const QString& jid)
{
    AccountsMap::const_iterator itr = amap.find(account);
    if (itr == amap.end()) {
        qDebug("An attempt to remove contact from non existing account, skipping");
        return;
    }
    
    itr->second->removeContactRequest(jid);
}

void Core::Squawk::addContactRequest(const QString& account, const QString& jid, const QString& name, const QSet<QString>& groups)
{
    AccountsMap::const_iterator itr = amap.find(account);
    if (itr == amap.end()) {
        qDebug("An attempt to add contact to a non existing account, skipping");
        return;
    }
    
    itr->second->addContactRequest(jid, name, groups);
}

void Core::Squawk::onAccountAddRoom(const QString jid, const QMap<QString, QVariant>& data)
{
    Account* acc = static_cast<Account*>(sender());
    emit addRoom(acc->getName(), jid, data);
}

void Core::Squawk::onAccountChangeRoom(const QString jid, const QMap<QString, QVariant>& data)
{
    Account* acc = static_cast<Account*>(sender());
    emit changeRoom(acc->getName(), jid, data);
}

void Core::Squawk::onAccountRemoveRoom(const QString jid)
{
    Account* acc = static_cast<Account*>(sender());
    emit removeRoom(acc->getName(), jid);
}

void Core::Squawk::setRoomJoined(const QString& account, const QString& jid, bool joined)
{
    AccountsMap::const_iterator itr = amap.find(account);
    if (itr == amap.end()) {
        qDebug() << "An attempt to set jouned to the room" << jid << "of non existing account" << account << ", skipping";
        return;
    }
    itr->second->setRoomJoined(jid, joined);
}

void Core::Squawk::setRoomAutoJoin(const QString& account, const QString& jid, bool joined)
{
    AccountsMap::const_iterator itr = amap.find(account);
    if (itr == amap.end()) {
        qDebug() << "An attempt to set autoJoin to the room" << jid << "of non existing account" << account << ", skipping";
        return;
    }
    itr->second->setRoomAutoJoin(jid, joined);
}

void Core::Squawk::onAccountAddRoomPresence(const QString& jid, const QString& nick, const QMap<QString, QVariant>& data)
{
    Account* acc = static_cast<Account*>(sender());
    emit addRoomParticipant(acc->getName(), jid, nick, data);
}

void Core::Squawk::onAccountChangeRoomPresence(const QString& jid, const QString& nick, const QMap<QString, QVariant>& data)
{
    Account* acc = static_cast<Account*>(sender());
    emit changeRoomParticipant(acc->getName(), jid, nick, data);
}

void Core::Squawk::onAccountRemoveRoomPresence(const QString& jid, const QString& nick)
{
    Account* acc = static_cast<Account*>(sender());
    emit removeRoomParticipant(acc->getName(), jid, nick);
}

void Core::Squawk::onAccountChangeMessage(const QString& jid, const QString& id, const QMap<QString, QVariant>& data)
{
    Account* acc = static_cast<Account*>(sender());
    emit changeMessage(acc->getName(), jid, id, data);
}

void Core::Squawk::removeRoomRequest(const QString& account, const QString& jid)
{
    AccountsMap::const_iterator itr = amap.find(account);
    if (itr == amap.end()) {
        qDebug() << "An attempt to remove the room" << jid << "of non existing account" << account << ", skipping";
        return;
    }
    itr->second->removeRoomRequest(jid);
}

void Core::Squawk::addRoomRequest(const QString& account, const QString& jid, const QString& nick, const QString& password, bool autoJoin)
{
    AccountsMap::const_iterator itr = amap.find(account);
    if (itr == amap.end()) {
        qDebug() << "An attempt to add the room" << jid << "to non existing account" << account << ", skipping";
        return;
    }
    itr->second->addRoomRequest(jid, nick, password, autoJoin);
}

void Core::Squawk::fileDownloadRequest(const QString& url)
{
    network.downladFile(url);
}

void Core::Squawk::addContactToGroupRequest(const QString& account, const QString& jid, const QString& groupName)
{
    AccountsMap::const_iterator itr = amap.find(account);
    if (itr == amap.end()) {
        qDebug() << "An attempt to add contact" << jid << "of non existing account" << account << "to the group" << groupName << ", skipping";
        return;
    }
    itr->second->addContactToGroupRequest(jid, groupName);
}

void Core::Squawk::removeContactFromGroupRequest(const QString& account, const QString& jid, const QString& groupName)
{
    AccountsMap::const_iterator itr = amap.find(account);
    if (itr == amap.end()) {
        qDebug() << "An attempt to add contact" << jid << "of non existing account" << account << "to the group" << groupName << ", skipping";
        return;
    }
    itr->second->removeContactFromGroupRequest(jid, groupName);
}

void Core::Squawk::renameContactRequest(const QString& account, const QString& jid, const QString& newName)
{
    AccountsMap::const_iterator itr = amap.find(account);
    if (itr == amap.end()) {
        qDebug() << "An attempt to rename contact" << jid << "of non existing account" << account << ", skipping";
        return;
    }
    itr->second->renameContactRequest(jid, newName);
}

void Core::Squawk::requestVCard(const QString& account, const QString& jid)
{
    AccountsMap::const_iterator itr = amap.find(account);
    if (itr == amap.end()) {
        qDebug() << "An attempt to request" << jid << "vcard of non existing account" << account << ", skipping";
        return;
    }
    itr->second->requestVCard(jid);
}

void Core::Squawk::uploadVCard(const QString& account, const Shared::VCard& card)
{
    AccountsMap::const_iterator itr = amap.find(account);
    if (itr == amap.end()) {
        qDebug() << "An attempt to upload vcard to non existing account" << account << ", skipping";
        return;
    }
    itr->second->uploadVCard(card);
}

void Core::Squawk::responsePassword(const QString& account, const QString& password)
{
    AccountsMap::const_iterator itr = amap.find(account);
    if (itr == amap.end()) {
        qDebug() << "An attempt to set password to non existing account" << account << ", skipping";
        return;
    }
    itr->second->setPassword(password);
    emit changeAccount(account, {{"password", password}});
    accountReady();
}

void Core::Squawk::readSettings()
{
    QSettings settings;
    settings.beginGroup("core");
    int size = settings.beginReadArray("accounts");
    waitingForAccounts = size;
    for (int i = 0; i < size; ++i) {
        settings.setArrayIndex(i);
        parseAccount(
            settings.value("login").toString(), 
            settings.value("server").toString(), 
            settings.value("password", "").toString(), 
            settings.value("name").toString(),
            settings.value("resource").toString(),
            Shared::Global::fromInt<Shared::AccountPassword>(settings.value("passwordType", static_cast<int>(Shared::AccountPassword::plain)).toInt())
        );
    }
    settings.endArray();
    settings.endGroup();
}

void Core::Squawk::accountReady()
{
    --waitingForAccounts;
    
    if (waitingForAccounts == 0) {
        emit ready();
    }
}

void Core::Squawk::parseAccount(
    const QString& login, 
    const QString& server, 
    const QString& password, 
    const QString& name, 
    const QString& resource, 
    Shared::AccountPassword passwordType
)
{
    switch (passwordType) {
        case Shared::AccountPassword::plain:
            addAccount(login, server, password, name, resource, passwordType);
            accountReady();
            break;
        case Shared::AccountPassword::jammed: {
            SimpleCrypt crypto(passwordHash);
            QString decrypted = crypto.decryptToString(password);
            addAccount(login, server, decrypted, name, resource, passwordType);
            accountReady();
        }
            break;
        case Shared::AccountPassword::alwaysAsk: 
            addAccount(login, server, QString(), name, resource, passwordType);
            emit requestPassword(name);
            break;
        case Shared::AccountPassword::kwallet: {
            addAccount(login, server, QString(), name, resource, passwordType);
#ifdef WITH_KWALLET
            if (kwallet.supportState() == PSE::KWallet::success) {
                kwallet.requestReadPassword(name);
            } else {
#endif
                emit requestPassword(name);
#ifdef WITH_KWALLET
            }
#endif
        }
    }
}

void Core::Squawk::onWalletRejectPassword(const QString& login)
{
    emit requestPassword(login);
}

void Core::Squawk::onWalletResponsePassword(const QString& login, const QString& password)
{
    AccountsMap::const_iterator itr = amap.find(login);
    if (itr == amap.end()) {
        qDebug() << "An attempt to set password to non existing account" << login << ", skipping";
        return;
    }
    itr->second->setPassword(password);
    emit changeAccount(login, {{"password", password}});
    accountReady();
}

void Core::Squawk::onAccountUploadFileError(const QString& jid, const QString id, const QString& errorText)
{
    Account* acc = static_cast<Account*>(sender());
    emit fileError({{acc->getName(), jid, id}}, errorText, true);
}

void Core::Squawk::onLocalPathInvalid(const QString& path)
{
    std::list<Shared::MessageInfo> list = network.reportPathInvalid(path);
    
    QMap<QString, QVariant> data({
        {"attachPath", ""}
    });
    for (const Shared::MessageInfo& info : list) {
        AccountsMap::const_iterator itr = amap.find(info.account);
        if (itr != amap.end()) {
            itr->second->requestChangeMessage(info.jid, info.messageId, data);
        } else {
            qDebug() << "Reacting on failure to reach file" << path << "there was an attempt to change message in account" << info.account << "which doesn't exist, skipping";
        }
    }
}
