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

#ifndef CORE_ACCOUNT_H
#define CORE_ACCOUNT_H

#include <QObject>
#include <QCryptographicHash>
#include <QFile>
#include <QMimeDatabase>
#include <QStandardPaths>
#include <QDir>
#include <QTimer>

#include <map>
#include <set>

#include <QXmppRosterManager.h>
#include <QXmppCarbonManager.h>
#include <QXmppDiscoveryManager.h>
#include <QXmppMamManager.h>
#include <QXmppMucManager.h>
#include <QXmppClient.h>
#include <QXmppBookmarkManager.h>
#include <QXmppBookmarkSet.h>
#include <QXmppUploadRequestManager.h>
#include <QXmppVCardIq.h>
#include <QXmppVCardManager.h>
#include <QXmppMessageReceiptManager.h>

#include "shared.h"
#include "contact.h"
#include "conference.h"
#include "networkaccess.h"

#include "handlers/messagehandler.h"
#include "handlers/rosterhandler.h"

namespace Core
{

class Account : public QObject
{
    Q_OBJECT
    friend class MessageHandler;
    friend class RosterHandler;
public:
    Account(
        const QString& p_login, 
        const QString& p_server, 
        const QString& p_password, 
        const QString& p_name, 
        NetworkAccess* p_net, 
        QObject* parent = 0);
    ~Account();
    
    Shared::ConnectionState getState() const;
    QString getName() const;
    QString getLogin() const;
    QString getServer() const;
    QString getPassword() const;
    QString getResource() const;
    QString getAvatarPath() const;
    Shared::Availability getAvailability() const;
    Shared::AccountPassword getPasswordType() const;
    
    void setName(const QString& p_name);
    void setLogin(const QString& p_login);
    void setServer(const QString& p_server);
    void setPassword(const QString& p_password);
    void setResource(const QString& p_resource);
    void setAvailability(Shared::Availability avail);
    void setPasswordType(Shared::AccountPassword pt);
    QString getFullJid() const;
    void sendMessage(const Shared::Message& data);
    void requestArchive(const QString& jid, int count, const QString& before);
    void subscribeToContact(const QString& jid, const QString& reason);
    void unsubscribeFromContact(const QString& jid, const QString& reason);
    void removeContactRequest(const QString& jid);
    void addContactRequest(const QString& jid, const QString& name, const QSet<QString>& groups);
    void addContactToGroupRequest(const QString& jid, const QString& groupName);
    void removeContactFromGroupRequest(const QString& jid, const QString& groupName);
    void renameContactRequest(const QString& jid, const QString& newName);
    
    void setRoomJoined(const QString& jid, bool joined);
    void setRoomAutoJoin(const QString& jid, bool joined);
    void removeRoomRequest(const QString& jid);
    void addRoomRequest(const QString& jid, const QString& nick, const QString& password, bool autoJoin);
    void uploadVCard(const Shared::VCard& card);
    
public slots:
    void connect();
    void disconnect();
    void reconnect();
    void requestVCard(const QString& jid);
    
signals:
    void changed(const QMap<QString, QVariant>& data);
    void connectionStateChanged(Shared::ConnectionState);
    void availabilityChanged(Shared::Availability);
    void addGroup(const QString& name);
    void removeGroup(const QString& name);
    void addRoom(const QString& jid, const QMap<QString, QVariant>& data);
    void changeRoom(const QString& jid, const QMap<QString, QVariant>& data);
    void removeRoom(const QString& jid);
    void addContact(const QString& jid, const QString& group, const QMap<QString, QVariant>& data);
    void removeContact(const QString& jid);
    void removeContact(const QString& jid, const QString& group);
    void changeContact(const QString& jid, const QMap<QString, QVariant>& data);
    void addPresence(const QString& jid, const QString& name, const QMap<QString, QVariant>& data);
    void removePresence(const QString& jid, const QString& name);
    void message(const Shared::Message& data);
    void changeMessage(const QString& jid, const QString& id, const QMap<QString, QVariant>& data);
    void responseArchive(const QString& jid, const std::list<Shared::Message>& list, bool last);
    void error(const QString& text);
    void addRoomParticipant(const QString& jid, const QString& nickName, const QMap<QString, QVariant>& data);
    void changeRoomParticipant(const QString& jid, const QString& nickName, const QMap<QString, QVariant>& data);
    void removeRoomParticipant(const QString& jid, const QString& nickName);
    void receivedVCard(const QString& jid, const Shared::VCard& card);
    void uploadFile(const QFileInfo& file, const QUrl& set, const QUrl& get, QMap<QString, QString> headers);
    void uploadFileError(const QString& jid, const QString& messageId, const QString& error);
    
private:
    QString name;
    std::map<QString, QString> archiveQueries;
    QXmppClient client;
    QXmppConfiguration config;
    QXmppPresence presence;
    Shared::ConnectionState state;
    QXmppCarbonManager* cm;
    QXmppMamManager* am;
    QXmppMucManager* mm;
    QXmppBookmarkManager* bm;
    QXmppRosterManager* rm;
    QXmppVCardManager* vm;
    QXmppUploadRequestManager* um;
    QXmppDiscoveryManager* dm;
    QXmppMessageReceiptManager* rcpm;
    bool reconnectScheduled;
    QTimer* reconnectTimer;
    
    std::set<QString> pendingVCardRequests;
    
    QString avatarHash;
    QString avatarType;
    bool ownVCardRequestInProgress;
    NetworkAccess* network;
    Shared::AccountPassword passwordType;
    
    MessageHandler* mh;
    RosterHandler* rh;
    
private slots:
    void onClientStateChange(QXmppClient::State state);
    void onClientError(QXmppClient::Error err);
    
    void onPresenceReceived(const QXmppPresence& presence);
    void onContactNeedHistory(const QString& before, const QString& after, const QDateTime& at);

    void onMamMessageReceived(const QString& bareJid, const QXmppMessage& message);
    void onMamResultsReceived(const QString &queryId, const QXmppResultSetReply &resultSetReply, bool complete);

    void onMamLog(QXmppLogger::MessageType type, const QString &msg);
    
    void onVCardReceived(const QXmppVCardIq& card);
    void onOwnVCardReceived(const QXmppVCardIq& card);

    void onDiscoveryItemsReceived (const QXmppDiscoveryIq& items);
    void onDiscoveryInfoReceived (const QXmppDiscoveryIq& info);
    void onContactHistoryResponse(const std::list<Shared::Message>& list, bool last);
  
private:
    void handleDisconnection();
    void onReconnectTimer();
};

void initializeVCard(Shared::VCard& vCard, const QXmppVCardIq& card);
void initializeQXmppVCard(QXmppVCardIq& card, const Shared::VCard& vCard);
}


#endif // CORE_ACCOUNT_H
