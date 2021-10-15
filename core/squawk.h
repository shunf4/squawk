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

#ifndef CORE_SQUAWK_H
#define CORE_SQUAWK_H

#include <QObject>
#include <QString>
#include <QVariant>
#include <QMap>
#include <QtGlobal>

#include <deque>

#include "account.h"
#include "shared/enums.h"
#include "shared/message.h"
#include "shared/global.h"
#include "networkaccess.h"
#include "external/simpleCrypt/simplecrypt.h"

#ifdef WITH_KWALLET
#include "passwordStorageEngines/kwallet.h"
#endif

namespace Core
{
class Squawk : public QObject
{
    Q_OBJECT

public:
    Squawk(QObject* parent = 0);
    ~Squawk();

signals:
    void quit();
    void ready();
    
    void newAccount(const QMap<QString, QVariant>&);
    void changeAccount(const QString& account, const QMap<QString, QVariant>& data);
    void removeAccount(const QString& account);
    
    void addGroup(const QString& account, const QString& name);
    void removeGroup(const QString& account, const QString& name);
    
    void addContact(const QString& account, const QString& jid, const QString& group, const QMap<QString, QVariant>& data);
    void removeContact(const QString& account, const QString& jid);
    void removeContact(const QString& account, const QString& jid, const QString& group);
    void changeContact(const QString& account, const QString& jid, const QMap<QString, QVariant>& data);
    
    void addPresence(const QString& account, const QString& jid, const QString& name, const QMap<QString, QVariant>& data);
    void removePresence(const QString& account, const QString& jid, const QString& name);
    
    void stateChanged(Shared::Availability state);
    
    void accountMessage(const QString& account, const Shared::Message& data);
    void responseArchive(const QString& account, const QString& jid, const std::list<Shared::Message>& list, bool last);
    
    void addRoom(const QString& account, const QString jid, const QMap<QString, QVariant>& data);
    void changeRoom(const QString& account, const QString jid, const QMap<QString, QVariant>& data);
    void removeRoom(const QString& account, const QString jid);
    void addRoomParticipant(const QString& account, const QString& jid, const QString& name, const QMap<QString, QVariant>& data);
    void changeRoomParticipant(const QString& account, const QString& jid, const QString& name, const QMap<QString, QVariant>& data);
    void removeRoomParticipant(const QString& account, const QString& jid, const QString& name);
    
    void fileError(const std::list<Shared::MessageInfo> msgs, const QString& error, bool up);
    void fileProgress(const std::list<Shared::MessageInfo> msgs, qreal value, bool up);
    void fileDownloadComplete(const std::list<Shared::MessageInfo> msgs, const QString& path);
    void fileUploadComplete(const std::list<Shared::MessageInfo> msgs, const QString& url, const QString& path);
    
    void responseVCard(const QString& jid, const Shared::VCard& card);
    void changeMessage(const QString& account, const QString& jid, const QString& id, const QMap<QString, QVariant>& data);
    void requestPassword(const QString& account);
    
public slots:
    void start();
    void stop();
    
    void newAccountRequest(const QMap<QString, QVariant>& map);
    void modifyAccountRequest(const QString& name, const QMap<QString, QVariant>& map);
    void removeAccountRequest(const QString& name);
    void connectAccount(const QString& account);
    void disconnectAccount(const QString& account);
    
    void changeState(Shared::Availability state);
    
    void sendMessage(const QString& account, const Shared::Message& data);
    void resendMessage(const QString& account, const QString& jid, const QString& id);
    void requestArchive(const QString& account, const QString& jid, int count, const QString& before);
    
    void subscribeContact(const QString& account, const QString& jid, const QString& reason);
    void unsubscribeContact(const QString& account, const QString& jid, const QString& reason);
    void addContactToGroupRequest(const QString& account, const QString& jid, const QString& groupName);
    void removeContactFromGroupRequest(const QString& account, const QString& jid, const QString& groupName);
    void removeContactRequest(const QString& account, const QString& jid);
    void renameContactRequest(const QString& account, const QString& jid, const QString& newName);
    void addContactRequest(const QString& account, const QString& jid, const QString& name, const QSet<QString>& groups);
    
    void setRoomJoined(const QString& account, const QString& jid, bool joined);
    void setRoomAutoJoin(const QString& account, const QString& jid, bool joined);
    void addRoomRequest(const QString& account, const QString& jid, const QString& nick, const QString& password, bool autoJoin);
    void removeRoomRequest(const QString& account, const QString& jid);
    
    void fileDownloadRequest(const QString& url);
    
    void requestVCard(const QString& account, const QString& jid);
    void uploadVCard(const QString& account, const Shared::VCard& card);
    void responsePassword(const QString& account, const QString& password);
    void onLocalPathInvalid(const QString& path);
    
private:
    typedef std::deque<Account*> Accounts;
    typedef std::map<QString, Account*> AccountsMap;
    
    Accounts accounts;
    AccountsMap amap;
    Shared::Availability state;
    NetworkAccess network;
    uint8_t waitingForAccounts;
    bool isInitialized;

#ifdef WITH_KWALLET
    PSE::KWallet kwallet;
#endif
    
private slots:
    void addAccount(
        const QString& login, 
        const QString& server, 
        const QString& password, 
        const QString& name, 
        const QString& resource, 
        Shared::AccountPassword passwordType
    );
    
    void onAccountConnectionStateChanged(Shared::ConnectionState state);
    void onAccountAvailabilityChanged(Shared::Availability state);
    void onAccountChanged(const QMap<QString, QVariant>& data);
    void onAccountAddGroup(const QString& name);
    void onAccountError(const QString& text);
    void onAccountRemoveGroup(const QString& name);
    void onAccountAddContact(const QString& jid, const QString& group, const QMap<QString, QVariant>& data);
    void onAccountRemoveContact(const QString& jid);
    void onAccountRemoveContact(const QString& jid, const QString& group);
    void onAccountChangeContact(const QString& jid, const QMap<QString, QVariant>& data);
    void onAccountAddPresence(const QString& jid, const QString& name, const QMap<QString, QVariant>& data);
    void onAccountRemovePresence(const QString& jid, const QString& name);
    void onAccountMessage(const Shared::Message& data);
    void onAccountResponseArchive(const QString& jid, const std::list<Shared::Message>& list, bool last);
    void onAccountAddRoom(const QString jid, const QMap<QString, QVariant>& data);
    void onAccountChangeRoom(const QString jid, const QMap<QString, QVariant>& data);
    void onAccountRemoveRoom(const QString jid);
    void onAccountAddRoomPresence(const QString& jid, const QString& nick, const QMap<QString, QVariant>& data);
    void onAccountChangeRoomPresence(const QString& jid, const QString& nick, const QMap<QString, QVariant>& data);
    void onAccountRemoveRoomPresence(const QString& jid, const QString& nick);
    void onAccountChangeMessage(const QString& jid, const QString& id, const QMap<QString, QVariant>& data);
    
    void onAccountUploadFileError(const QString& jid, const QString id, const QString& errorText);
    
    void onWalletOpened(bool success);
    void onWalletResponsePassword(const QString& login, const QString& password);
    void onWalletRejectPassword(const QString& login);
    
private:
    void readSettings();
    void accountReady();
    void parseAccount(
        const QString& login, 
        const QString& server, 
        const QString& password, 
        const QString& name, 
        const QString& resource, 
        Shared::AccountPassword passwordType
    );
    
    static const quint64 passwordHash = 0x3c5bb27570f50e11;
};

}

#endif // CORE_SQUAWK_H
