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
#include <QXmppHttpUploadIq.h>
#include <QXmppVCardIq.h>
#include <QXmppVCardManager.h>
#include "global.h"
#include "contact.h"
#include "conference.h"
#include "networkaccess.h"

namespace Core
{

class Account : public QObject
{
    Q_OBJECT
public:
    Account(const QString& p_login, const QString& p_server, const QString& p_password, const QString& p_name, NetworkAccess* p_net, QObject* parent = 0);
    ~Account();
    
    void connect();
    void disconnect();
    void reconnect();
    
    Shared::ConnectionState getState() const;
    QString getName() const;
    QString getLogin() const;
    QString getServer() const;
    QString getPassword() const;
    QString getResource() const;
    QString getAvatarPath() const;
    Shared::Availability getAvailability() const;
    
    void setName(const QString& p_name);
    void setLogin(const QString& p_login);
    void setServer(const QString& p_server);
    void setPassword(const QString& p_password);
    void setResource(const QString& p_resource);
    void setAvailability(Shared::Availability avail);
    QString getFullJid() const;
    void sendMessage(const Shared::Message& data);
    void sendMessage(const Shared::Message& data, const QString& path);
    void requestArchive(const QString& jid, int count, const QString& before);
    void setReconnectTimes(unsigned int times);
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
    void requestVCard(const QString& jid);
    
signals:
    void changed(const QMap<QString, QVariant>& data);
    void connectionStateChanged(int);
    void availabilityChanged(int);
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
    void responseArchive(const QString& jid, const std::list<Shared::Message>& list);
    void error(const QString& text);
    void addRoomParticipant(const QString& jid, const QString& nickName, const QMap<QString, QVariant>& data);
    void changeRoomParticipant(const QString& jid, const QString& nickName, const QMap<QString, QVariant>& data);
    void removeRoomParticipant(const QString& jid, const QString& nickName);
    void receivedVCard(const QString& jid, const Shared::VCard& card);
    void uploadFile(const QFileInfo& file, const QUrl& set, const QUrl& get, QMap<QString, QString> headers);
    void uploadFileError(const QString& messageId, const QString& error);
    
private:
    QString name;
    std::map<QString, QString> achiveQueries;
    QXmppClient client;
    QXmppConfiguration config;
    QXmppPresence presence;
    Shared::ConnectionState state;
    std::map<QString, std::set<QString>> groups;
    QXmppCarbonManager* cm;
    QXmppMamManager* am;
    QXmppMucManager* mm;
    QXmppBookmarkManager* bm;
    QXmppRosterManager* rm;
    QXmppVCardManager* vm;
    QXmppUploadRequestManager* um;
    QXmppDiscoveryManager* dm;
    std::map<QString, Contact*> contacts;
    std::map<QString, Conference*> conferences;
    unsigned int maxReconnectTimes;
    unsigned int reconnectTimes;
    
    std::map<QString, QString> queuedContacts;
    std::set<QString> outOfRosterContacts;
    std::set<QString> pendingVCardRequests;
    std::map<QString, Shared::Message> pendingMessages;
    std::deque<std::pair<QString, Shared::Message>> uploadingSlotsQueue;
    
    QString avatarHash;
    QString avatarType;
    bool ownVCardRequestInProgress;
    NetworkAccess* network;
    
private slots:
    void onClientConnected();
    void onClientDisconnected();
    void onClientError(QXmppClient::Error err);
    
    void onRosterReceived();
    void onRosterItemAdded(const QString& bareJid);
    void onRosterItemChanged(const QString& bareJid);
    void onRosterItemRemoved(const QString& bareJid);
    void onRosterPresenceChanged(const QString& bareJid, const QString& resource);
    
    void onPresenceReceived(const QXmppPresence& presence);
    void onMessageReceived(const QXmppMessage& message);
    
    void onCarbonMessageReceived(const QXmppMessage& message);
    void onCarbonMessageSent(const QXmppMessage& message);
    
    void onMamMessageReceived(const QString& bareJid, const QXmppMessage& message);
    void onMamResultsReceived(const QString &queryId, const QXmppResultSetReply &resultSetReply, bool complete);
    
    void onMucRoomAdded(QXmppMucRoom* room);
    void onMucJoinedChanged(bool joined);
    void onMucAutoJoinChanged(bool autoJoin);
    void onMucNickNameChanged(const QString& nickName);
    void onMucSubjectChanged(const QString& subject);
    void onMucAddParticipant(const QString& nickName, const QMap<QString, QVariant>& data);
    void onMucChangeParticipant(const QString& nickName, const QMap<QString, QVariant>& data);
    void onMucRemoveParticipant(const QString& nickName);
    
    void bookmarksReceived(const QXmppBookmarkSet& bookmarks);
    
    void onContactGroupAdded(const QString& group);
    void onContactGroupRemoved(const QString& group);
    void onContactNameChanged(const QString& name);
    void onContactSubscriptionStateChanged(Shared::SubscriptionState state);
    void onContactHistoryResponse(const std::list<Shared::Message>& list);
    void onContactNeedHistory(const QString& before, const QString& after, const QDateTime& at);
    void onContactAvatarChanged(Shared::Avatar, const QString& path);
    
    void onMamLog(QXmppLogger::MessageType type, const QString &msg);
    
    void onVCardReceived(const QXmppVCardIq& card);
    void onOwnVCardReceived(const QXmppVCardIq& card);
    
    void onUploadSlotReceived(const QXmppHttpUploadSlotIq& slot);
    void onUploadSlotRequestFailed(const QXmppHttpUploadRequestIq& request);
    void onFileUploaded(const QString& messageId, const QString& url);
    void onFileUploadError(const QString& messageId, const QString& errMsg);
    void onDiscoveryItemsReceived (const QXmppDiscoveryIq& items);
    void onDiscoveryInfoReceived (const QXmppDiscoveryIq& info);
  
private:
    void addedAccount(const QString &bareJid);
    void handleNewContact(Contact* contact);
    void handleNewRosterItem(RosterItem* contact);
    void handleNewConference(Conference* contact);
    bool handleChatMessage(const QXmppMessage& msg, bool outgoing = false, bool forwarded = false, bool guessing = false);
    bool handleGroupMessage(const QXmppMessage& msg, bool outgoing = false, bool forwarded = false, bool guessing = false);
    void addNewRoom(const QString& jid, const QString& nick, const QString& roomName, bool autoJoin);
    void addToGroup(const QString& jid, const QString& group);
    void removeFromGroup(const QString& jid, const QString& group);
    void initializeMessage(Shared::Message& target, const QXmppMessage& source, bool outgoing = false, bool forwarded = false, bool guessing = false) const;
    Shared::SubscriptionState castSubscriptionState(QXmppRosterIq::Item::SubscriptionType qs) const;
    void logMessage(const QXmppMessage& msg, const QString& reason = "Message wasn't handled: ");
    void storeConferences();
    void clearConferences();
    void sendMessageWithLocalUploadedFile(Shared::Message msg, const QString& url);
};

void initializeVCard(Shared::VCard& vCard, const QXmppVCardIq& card);
void initializeQXmppVCard(QXmppVCardIq& card, const Shared::VCard& vCard);
}


#endif // CORE_ACCOUNT_H
