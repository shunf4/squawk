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

#include "room.h"
#include "shared/icons.h"

#include <QIcon>
#include <QDebug>

Models::Room::Room(const Account* acc, const QString& p_jid, const QMap<QString, QVariant>& data, Models::Item* parentItem):
    Element(room, acc, p_jid, data, parentItem),
    autoJoin(false),
    joined(false),
    nick(""),
    subject(""),
    participants(),
    exParticipantAvatars()
{
    QMap<QString, QVariant>::const_iterator itr = data.find("autoJoin");
    if (itr != data.end()) {
        setAutoJoin(itr.value().toBool());
    }
    
    itr = data.find("joined");
    if (itr != data.end()) {
        setJoined(itr.value().toBool());
    }
    
    itr = data.find("nick");
    if (itr != data.end()) {
        setNick(itr.value().toString());
    }
    
    itr = data.find("subject");
    if (itr != data.end()) {
        setSubject(itr.value().toString());
    }
    
    itr = data.find("avatars");
    if (itr != data.end()) {
        QMap<QString, QVariant> avs = itr.value().toMap();
        for (QMap<QString, QVariant>::const_iterator itr = avs.begin(), end = avs.end(); itr != end; ++itr) {
            exParticipantAvatars.insert(std::make_pair(itr.key(), itr.value().toString()));
        }
    }
}

Models::Room::~Room()
{
}

int Models::Room::columnCount() const
{
    return 7;
}

bool Models::Room::getAutoJoin() const
{
    return autoJoin;
}

bool Models::Room::getJoined() const
{
    return joined;
}

QString Models::Room::getNick() const
{
    return nick;
}

QString Models::Room::getRoomName() const
{
    if (name.size() == 0) {
        return jid;
    } else {
        return name;
    }
}

QVariant Models::Room::data(int column) const
{
    switch (column) {
        case 0:
            return getRoomName();
        case 1:
            return jid;
        case 2:
            return getJoined();
        case 3:
            return getAutoJoin();
        case 4:
            return getNick();
        case 5:
            return getMessagesCount();
        case 6:
            return getSubject();
        case 7:
            return static_cast<quint8>(getAvatarState());
        case 8:
            return getAvatarPath();
        default:
            return QVariant();
    }
}

void Models::Room::setAutoJoin(bool p_autoJoin)
{
    if (autoJoin != p_autoJoin) {
        autoJoin = p_autoJoin;
        changed(3);
    }
}

void Models::Room::setJoined(bool p_joined)
{
    if (joined != p_joined) {
        joined = p_joined;
        changed(2);
        if (!joined) {
            toOfflineState();
        }
    }
}

void Models::Room::setNick(const QString& p_nick)
{
    if (nick != p_nick) {
        nick = p_nick;
        changed(4);
    }
}

void Models::Room::update(const QString& field, const QVariant& value)
{
    if (field == "name") {
        setName(value.toString());
    } else if (field == "joined") {
        setJoined(value.toBool());
    } else if (field == "autoJoin") {
        setAutoJoin(value.toBool());
    } else if (field == "nick") {
        setNick(value.toString());
    } else if (field == "subject") {
        setSubject(value.toString());
    } else {
        Element::update(field, value);
    }
}

QIcon Models::Room::getStatusIcon(bool big) const
{
    if (getMessagesCount() > 0) {
        return Shared::icon("mail-message", big);
    } else {
        if (autoJoin) {
            if (joined) {
                return Shared::connectionStateIcon(Shared::ConnectionState::connected, big);
            } else {
                return Shared::connectionStateIcon(Shared::ConnectionState::disconnected, big);
            }
        } else {
            if (joined) {
                return Shared::connectionStateIcon(Shared::ConnectionState::connecting, big);
            } else {
                return Shared::connectionStateIcon(Shared::ConnectionState::error, big);
            }
        }
    }
}

QString Models::Room::getStatusText() const
{
    if (autoJoin) {
        if (joined) {
            return tr("Subscribed");
        } else {
            return tr("Temporarily unsubscribed");
        }
    } else {
        if (joined) {
            return tr("Temporarily subscribed");
        } else {
            return tr("Unsubscribed");
        }
    }
}


void Models::Room::toOfflineState()
{
    emit childIsAboutToBeRemoved(this, 0, childItems.size());
    for (std::deque<Item*>::size_type i = 0; i < childItems.size(); ++i) {
        Item* item = childItems[i];
        Item::_removeChild(i);
        item->deleteLater();
    }
    childItems.clear();
    participants.clear();
    emit childRemoved();
}

void Models::Room::addParticipant(const QString& p_name, const QMap<QString, QVariant>& data)
{
    std::map<QString, Participant*>::const_iterator itr = participants.find(p_name);
    if (itr != participants.end()) {
        qDebug() << "An attempt to add already existing participant" << p_name << "to the room" << name << ", updating instead";
        handleParticipantUpdate(itr, data);
    } else {
        std::map<QString, QString>::const_iterator eitr = exParticipantAvatars.find(name);
        if (eitr != exParticipantAvatars.end()) {
            exParticipantAvatars.erase(eitr);
        }
        
        Participant* part = new Participant(data);
        part->setName(p_name);
        participants.insert(std::make_pair(p_name, part));
        appendChild(part);
        emit participantJoined(*part);
    }
}

void Models::Room::changeParticipant(const QString& p_name, const QMap<QString, QVariant>& data)
{
    std::map<QString, Participant*>::const_iterator itr = participants.find(p_name);
    if (itr == participants.end()) {
        qDebug() << "An attempt to change non existing participant" << p_name << "from the room" << name << ", skipping";
    } else {
        handleParticipantUpdate(itr, data);
    }
}

void Models::Room::removeParticipant(const QString& p_name)
{
    std::map<QString, Participant*>::const_iterator itr = participants.find(p_name);
    if (itr == participants.end()) {
        qDebug() << "An attempt to remove non existing participant" << p_name << "from the room" << name << ", skipping";
    } else {
        Participant* p = itr->second;
        participants.erase(itr);
        removeChild(p->row());
        
        if (p->getAvatarState() != Shared::Avatar::empty) {
            exParticipantAvatars.insert(std::make_pair(p_name, p->getAvatarPath()));
        }
        
        p->deleteLater();
        emit participantLeft(p_name);
    }
}

void Models::Room::handleParticipantUpdate(std::map<QString, Participant*>::const_iterator itr, const QMap<QString, QVariant>& data)
{
    Participant* part = itr->second;
    const QString& p_name = itr->first;
    for (QMap<QString, QVariant>::const_iterator itr = data.begin(), end = data.end(); itr != end; ++itr) {
        part->update(itr.key(), itr.value());
    }
    if (p_name != part->getName()) {
        participants.erase(itr);
        participants.insert(std::make_pair(part->getName(), part));
    }
}

QString Models::Room::getSubject() const
{
    return subject;
}

void Models::Room::setSubject(const QString& sub)
{
    if (sub != subject) {
        subject = sub;
        changed(6);
    }
}

QString Models::Room::getDisplayedName() const
{
    return getRoomName();
}

std::map<QString, const Models::Participant &> Models::Room::getParticipants() const
{
    std::map<QString, const Models::Participant&> result;
    
    for (std::pair<QString, Models::Participant*> pair : participants) {
        result.emplace(pair.first, *(pair.second));
    }
    
    return result;
}

QString Models::Room::getParticipantIconPath(const QString& name) const
{
    std::map<QString, Models::Participant*>::const_iterator itr = participants.find(name);
    if (itr == participants.end()) {
        return "";
    }
    
    return itr->second->getAvatarPath();
}

std::map<QString, QString> Models::Room::getExParticipantAvatars() const
{
    return exParticipantAvatars;
}
