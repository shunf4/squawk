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

#ifndef CORE_ROSTERHANDLER_H
#define CORE_ROSTERHANDLER_H

#include <QObject>
#include <QSet>
#include <QString>
#include <QDateTime>

#include <list>
#include <map>
#include <set>

#include <QXmppBookmarkSet.h>
#include <QXmppMucManager.h>
#include <QXmppRosterIq.h>

#include <shared/message.h>
#include <core/contact.h>
#include <core/conference.h>

namespace Core {

    
class Account;
    
class RosterHandler : public QObject
{
    Q_OBJECT
public:
    RosterHandler(Account* account);
    ~RosterHandler();
    
    void addContactRequest(const QString& jid, const QString& name, const QSet<QString>& groups);
    void removeContactRequest(const QString& jid);
    void addContactToGroupRequest(const QString& jid, const QString& groupName);
    void removeContactFromGroupRequest(const QString& jid, const QString& groupName);
    
    void removeRoomRequest(const QString& jid);
    void addRoomRequest(const QString& jid, const QString& nick, const QString& password, bool autoJoin);
    void handleOffline();
    
    Core::Contact* getContact(const QString& jid);
    Core::Conference* getConference(const QString& jid);
    Core::RosterItem* getRosterItem(const QString& jid);
    Core::Contact* addOutOfRosterContact(const QString& jid);
    
    void storeConferences();
    void clearConferences();
    
private slots:
    void onRosterReceived();
    void onRosterItemAdded(const QString& bareJid);
    void onRosterItemChanged(const QString& bareJid);
    void onRosterItemRemoved(const QString& bareJid);
    
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
    void onContactAvatarChanged(Shared::Avatar, const QString& path);
    
private:
    void addNewRoom(const QString& jid, const QString& nick, const QString& roomName, bool autoJoin);
    void addToGroup(const QString& jid, const QString& group);
    void removeFromGroup(const QString& jid, const QString& group);
    void addedAccount(const QString &bareJid);
    void handleNewRosterItem(Core::RosterItem* contact);
    void handleNewContact(Core::Contact* contact);
    void handleNewConference(Core::Conference* contact);
    void careAboutAvatar(Core::RosterItem* item, QMap<QString, QVariant>& data);
    
    static Shared::SubscriptionState castSubscriptionState(QXmppRosterIq::Item::SubscriptionType qs);
    
private:
    Account* acc;
    std::map<QString, Contact*> contacts;
    std::map<QString, Conference*> conferences;
    std::map<QString, std::set<QString>> groups;
    std::map<QString, QString> queuedContacts;
    std::set<QString> outOfRosterContacts;
};

}

#endif // CORE_ROSTERHANDLER_H
