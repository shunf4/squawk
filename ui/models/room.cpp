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
#include <QIcon>

Models::Room::Room(const QString& p_jid, const QMap<QString, QVariant>& data, Models::Item* parentItem):
    Item(room, data, parentItem),
    autoJoin(false),
    joined(false),
    jid(p_jid),
    nick(""),
    messages()
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
}

Models::Room::~Room()
{
}

unsigned int Models::Room::getUnreadMessagesCount() const
{
    return messages.size();
}

int Models::Room::columnCount() const
{
    return 6;
}

QString Models::Room::getJid() const
{
    return jid;
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

void Models::Room::setJid(const QString& p_jid)
{
    if (jid != p_jid) {
        jid = p_jid;
        changed(1);
    }
}

void Models::Room::setJoined(bool p_joined)
{
    if (joined != p_joined) {
        joined = p_joined;
        changed(2);
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
    } else if (field == "jid") {
        setJid(value.toString());
    } else if (field == "joined") {
        setJoined(value.toBool());
    } else if (field == "autoJoin") {
        setAutoJoin(value.toBool());
    } else if (field == "nick") {
        setNick(value.toString());
    }
}

QIcon Models::Room::getStatusIcon(bool big) const
{
    if (messages.size() > 0) {
        return Shared::icon("mail-message", big);
    } else {
        if (autoJoin) {
            if (joined) {
                return Shared::connectionStateIcon(Shared::connected, big);
            } else {
                return Shared::connectionStateIcon(Shared::disconnected, big);
            }
        } else {
            if (joined) {
                return Shared::connectionStateIcon(Shared::connecting, big);
            } else {
                return Shared::connectionStateIcon(Shared::error, big);
            }
        }
    }
}

QString Models::Room::getStatusText() const
{
    if (autoJoin) {
        if (joined) {
            return "Subscribed";
        } else {
            return "Temporarily unsubscribed";
        }
    } else {
        if (joined) {
            return "Temporarily subscribed";
        } else {
            return "Unsubscribed";
        }
    }
}

unsigned int Models::Room::getMessagesCount() const
{
    return messages.size();
}

void Models::Room::addMessage(const Shared::Message& data)
{
    messages.emplace_back(data);
    changed(5);
}

void Models::Room::dropMessages()
{
    if (messages.size() > 0) {
        messages.clear();
        changed(5);
    }
}

void Models::Room::getMessages(Models::Room::Messages& container) const
{
    for (Messages::const_iterator itr = messages.begin(), end = messages.end(); itr != end; ++itr) {
        const Shared::Message& msg = *itr;
        container.push_back(msg);
    }
}
