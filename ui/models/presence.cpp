/*
 * <one line to give the program's name and a brief idea of what it does.>
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

#include "presence.h"

Models::Presence::Presence(const QMap<QString, QVariant>& data, Item* parentItem):
    Item(Item::presence, data, parentItem),
    availability(Shared::offline),
    lastActivity(data.value("lastActivity").toDateTime()),
    status(data.value("status").toString()),
    messages()
{
    QMap<QString, QVariant>::const_iterator itr = data.find("availability");
    if (itr != data.end()) {
        setAvailability(itr.value().toUInt());
    }
}

Models::Presence::~Presence()
{
}

int Models::Presence::columnCount() const
{
    return 5;
}

QVariant Models::Presence::data(int column) const
{
    switch (column) {
        case 0:
            return Item::data(column);
        case 1:
            return lastActivity;
        case 2:
            return availability;
        case 3:
            return status;
        case 4:
            return getMessagesCount();
        default:
            return QVariant();
    }
}

Shared::Availability Models::Presence::getAvailability() const
{
    return availability;
}

QDateTime Models::Presence::getLastActivity() const
{
    return lastActivity;
}

QString Models::Presence::getStatus() const
{
    return status;
}

void Models::Presence::setAvailability(Shared::Availability p_avail)
{
    if (availability != p_avail) {
        availability = p_avail;
        changed(2);
    }
}

void Models::Presence::setAvailability(unsigned int avail)
{
    if (avail <= Shared::availabilityHighest) {
        Shared::Availability state = static_cast<Shared::Availability>(avail);
        setAvailability(state);
    } else {
        qDebug("An attempt to set wrong state to the contact");
    }
}


void Models::Presence::setLastActivity(const QDateTime& p_time)
{
    if (lastActivity != p_time) {
        lastActivity = p_time;
        changed(1);
    }
}

void Models::Presence::setStatus(const QString& p_state)
{
    if (status != p_state) {
        status = p_state;
        changed(3);
    }
}

void Models::Presence::update(const QString& key, const QVariant& value)
{
    if (key == "name") {
        setName(value.toString());
    } else if (key == "status") {
        setStatus(value.toString());
    } else if (key == "availability") {
        setAvailability(value.toUInt());
    } else if (key == "lastActivity") {
        setLastActivity(value.toDateTime());
    }
}

unsigned int Models::Presence::getMessagesCount() const
{
    return messages.size();
}

void Models::Presence::addMessage(const Shared::Message& data)
{
    messages.emplace_back(data);
    changed(4);
}

void Models::Presence::dropMessages()
{
    if (messages.size() > 0) {
        messages.clear();
        changed(4);
    }
}

QIcon Models::Presence::getStatusIcon(bool big) const
{
    if (getMessagesCount() > 0) {
        return QIcon::fromTheme("mail-message");
    } else {
        return Shared::availabilityIcon(availability, big);
    }
}

void Models::Presence::getMessages(Models::Presence::Messages& container) const
{
    for (Messages::const_iterator itr = messages.begin(), end = messages.end(); itr != end; ++itr) {
        const Shared::Message& msg = *itr;
        container.push_back(msg);
    }
}
