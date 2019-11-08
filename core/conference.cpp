/*
 * Squawk messenger.
 * Copyright (C) 2019 Yury Gubich <blue@macaw.me>
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

#include "conference.h"

#include <QDebug>

Core::Conference::Conference(const QString& p_jid, const QString& p_account, bool p_autoJoin, const QString& p_name, const QString& p_nick, QXmppMucRoom* p_room):
    RosterItem(p_jid, p_account),
    nick(p_nick),
    room(p_room),
    joined(false),
    autoJoin(p_autoJoin)
{
    muc = true;
    name = p_name;
    
    connect(room, &QXmppMucRoom::joined, this, &Conference::onRoomJoined);
    connect(room, &QXmppMucRoom::left, this, &Conference::onRoomLeft);
    connect(room, &QXmppMucRoom::nameChanged, this, &Conference::onRoomNameChanged);
    connect(room, &QXmppMucRoom::subjectChanged, this, &Conference::onRoomSubjectChanged);
    connect(room, &QXmppMucRoom::participantAdded, this, &Conference::onRoomParticipantAdded);
    connect(room, &QXmppMucRoom::participantChanged, this, &Conference::onRoomParticipantChanged);
    connect(room, &QXmppMucRoom::participantRemoved, this, &Conference::onRoomParticipantRemoved);
    connect(room, &QXmppMucRoom::nickNameChanged, this, &Conference::onRoomNickNameChanged);
    connect(room, &QXmppMucRoom::error, this, &Conference::onRoomError);
    
    room->setNickName(nick);
    if (autoJoin) {
        room->join();
    }
}

Core::Conference::~Conference()
{
    if (joined) {
        room->leave();
    }
    room->deleteLater();
}

QString Core::Conference::getNick() const
{
    return nick;
}

bool Core::Conference::getAutoJoin()
{
    return autoJoin;
}

bool Core::Conference::getJoined() const
{
    return joined;
}

void Core::Conference::setJoined(bool p_joined)
{
    if (joined != p_joined) {
        if (p_joined) {
            room->join();
        } else {
            room->leave();
        }
    }
}

void Core::Conference::setAutoJoin(bool p_autoJoin)
{
    if (autoJoin != p_autoJoin) {
        autoJoin = p_autoJoin;
        emit autoJoinChanged(autoJoin);
    }
}

void Core::Conference::setNick(const QString& p_nick)
{
    if (nick != p_nick) {
        if (joined) {
            room->setNickName(p_nick);
        } else {
            nick = p_nick;
            emit nickChanged(nick);
        }
    }
}

void Core::Conference::onRoomJoined()
{
    joined = true;
    emit joinedChanged(joined);
}

void Core::Conference::onRoomLeft()
{
    joined = false;
    emit joinedChanged(joined);
}

void Core::Conference::onRoomNameChanged(const QString& p_name)
{
    setName(p_name);
}

void Core::Conference::onRoomNickNameChanged(const QString& p_nick)
{
    if (p_nick != nick) {
        nick = p_nick;
        emit nickChanged(nick);
    }
}

void Core::Conference::onRoomError(const QXmppStanza::Error& err)
{
    qDebug() << "MUC" << jid << "error:" << err.text();
}

void Core::Conference::onRoomParticipantAdded(const QString& p_name)
{
    QStringList comps = p_name.split("/");
    QString resource = comps.back();
    if (resource == jid) {
        qDebug() << "Room" << jid << "is reporting of adding itself to the list participants. Not sure what to do with that yet, skipping";
    } else {
        QXmppPresence pres = room->participantPresence(p_name);
        QDateTime lastInteraction = pres.lastUserInteraction();
        if (!lastInteraction.isValid()) {
            lastInteraction = QDateTime::currentDateTime();
        }
        QXmppMucItem mi = pres.mucItem();
        
        emit addParticipant(resource, {
            {"lastActivity", lastInteraction},
            {"availability", pres.availableStatusType()},
            {"status", pres.statusText()},
            {"affiliation", mi.affiliation()},
            {"role", mi.role()}
        });
    }
}

void Core::Conference::onRoomParticipantChanged(const QString& p_name)
{
    QStringList comps = p_name.split("/");
    QString resource = comps.back();
    if (resource == jid) {
        qDebug() << "Room" << jid << "is reporting of changing his own presence. Not sure what to do with that yet, skipping";
    } else {
        QXmppPresence pres = room->participantPresence(p_name);
        QDateTime lastInteraction = pres.lastUserInteraction();
        if (!lastInteraction.isValid()) {
            lastInteraction = QDateTime::currentDateTime();
        }
        QXmppMucItem mi = pres.mucItem();
        
        emit changeParticipant(resource, {
            {"lastActivity", lastInteraction},
            {"availability", pres.availableStatusType()},
            {"status", pres.statusText()},
            {"affiliation", mi.affiliation()},
            {"role", mi.role()}
        });
    }
}

void Core::Conference::onRoomParticipantRemoved(const QString& p_name)
{
    QStringList comps = p_name.split("/");
    QString resource = comps.back();
    if (resource == jid) {
        qDebug() << "Room" << jid << "is reporting of removing his own presence from the list of participants. Not sure what to do with that yet, skipping";
    } else {
        emit removeParticipant(resource);
    }
}

QString Core::Conference::getSubject() const
{
    if (joined) {
        return room->subject();
    } else {
        return "";
    }
}

void Core::Conference::onRoomSubjectChanged(const QString& p_name)
{
    emit subjectChanged(p_name);
}
