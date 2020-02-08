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

#include "presence.h"

Models::Presence::Presence(const QMap<QString, QVariant>& data, Item* parentItem):
    AbstractParticipant(Item::presence, data, parentItem),
    messages()
{
}

Models::Presence::Presence(const Models::Presence& other):
    AbstractParticipant(other),
    messages(other.messages)
{
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
        case 4:
            return getMessagesCount();
        default:
            return AbstractParticipant::data(column);
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

bool Models::Presence::changeMessage(const QString& id, const QMap<QString, QVariant>& data)
{
    bool found = false;
    for (Shared::Message& msg : messages) {
        if (msg.getId() == id) {
            msg.change(data);
            found = true;
            break;
        }
    }
    return found;
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
        return Shared::icon("mail-message", big);
    } else {
        return AbstractParticipant::getStatusIcon();
    }
}

void Models::Presence::getMessages(Models::Presence::Messages& container) const
{
    for (Messages::const_iterator itr = messages.begin(), end = messages.end(); itr != end; ++itr) {
        const Shared::Message& msg = *itr;
        container.push_back(msg);
    }
}
