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

#include "group.h"
#include "contact.h"
#include "reference.h"

Models::Group::Group(const QMap<QString, QVariant>& data, Models::Item* parentItem):
    Item(group, data, parentItem),
    unreadMessages(0)
{
}

Models::Group::~Group()
{
}

void Models::Group::appendChild(Models::Item* child)
{
    Item::appendChild(child);
    connect(child, &Item::childChanged, this, &Group::refresh);
    changed(1);
    refresh();
}

int Models::Group::columnCount() const
{
    return 3;
}

QVariant Models::Group::data(int column) const
{
    switch (column) {
        case 0:
            return getName();
        case 1:
            return QVariant((unsigned int)childItems.size());
        case 2:
            return unreadMessages;
        default:
            return QVariant();
    }
}

void Models::Group::_removeChild(int index)
{
    Item* child = childItems[index];
    disconnect(child, &Item::childChanged, this, &Group::refresh);
    Item::_removeChild(index);
    changed(1);
    refresh();
}

unsigned int Models::Group::getUnreadMessages() const
{
    return unreadMessages;
}

void Models::Group::setUnreadMessages(unsigned int amount)
{
    if (unreadMessages != amount) {
        unreadMessages = amount;
        changed(2);
    }
}

void Models::Group::refresh()
{
    unsigned int newAmount(0);
    
    for (std::deque<Models::Item*>::const_iterator itr = childItems.begin(), end = childItems.end(); itr != end; ++itr) {
        Models::Contact* cnt = static_cast<Models::Contact*>(*itr);
        newAmount += cnt->getMessagesCount();
    }
    
    setUnreadMessages(newAmount);
}

unsigned int Models::Group::getOnlineContacts() const
{
    unsigned int amount(0);
    
    for (std::deque<Models::Item*>::const_iterator itr = childItems.begin(), end = childItems.end(); itr != end; ++itr) {
        Models::Contact* cnt = static_cast<Models::Contact*>(*itr);
        if (cnt->getAvailability() != Shared::Availability::offline) {
            ++amount;
        }
    }
    
    return amount;
}
