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

#include "contact.h"

#include <QDebug>

Models::Contact::Contact(const Account* acc, const QString& p_jid ,const QMap<QString, QVariant> &data, Item *parentItem):
    Element(Item::contact, acc, p_jid, data, parentItem),
    availability(Shared::Availability::offline),
    state(Shared::SubscriptionState::none),
    presences(),
    status()
{
    QMap<QString, QVariant>::const_iterator itr = data.find("state");
    if (itr != data.end()) {
        setState(itr.value().toUInt());
    }
}

Models::Contact::~Contact()
{
}

void Models::Contact::setAvailability(unsigned int p_state)
{
    setAvailability(Shared::Global::fromInt<Shared::Availability>(p_state));
}

void Models::Contact::setState(unsigned int p_state)
{
    setState(Shared::Global::fromInt<Shared::SubscriptionState>(p_state));
}

Shared::Availability Models::Contact::getAvailability() const
{
    return availability;
}

void Models::Contact::setAvailability(Shared::Availability p_state)
{
    if (availability != p_state) {
        availability = p_state;
        changed(3);
    }
}

QString Models::Contact::getStatus() const
{
    return status;
}

void Models::Contact::setStatus(const QString& p_state)
{
    if (status != p_state) {
        status = p_state;
        changed(5);
    }
}

int Models::Contact::columnCount() const
{
    return 8;
}

QVariant Models::Contact::data(int column) const
{
    switch (column) {
        case 0:
            return getContactName();
        case 1:
            return jid;
        case 2:
            return QVariant::fromValue(state);
        case 3:
            return QVariant::fromValue(availability);
        case 4:
            return getMessagesCount();
        case 5:
            return getStatus();
        case 6:
            return QVariant::fromValue(getAvatarState());
        case 7:
            return getAvatarPath();
        default:
            return QVariant();
    }
}

QString Models::Contact::getContactName() const
{
    if (name == "") {
        return jid;
    } else {
        return name;
    }
}

void Models::Contact::update(const QString& field, const QVariant& value)
{
    if (field == "name") {
        setName(value.toString());
    } else if (field == "availability") {
        setAvailability(value.toUInt());
    } else if (field == "state") {
        setState(value.toUInt());
    } else {
        Element::update(field, value);
    }
}

void Models::Contact::addPresence(const QString& p_name, const QMap<QString, QVariant>& data)
{
    QMap<QString, Presence*>::iterator itr = presences.find(p_name);
    
    if (itr == presences.end()) {
        Presence* pr = new Presence(data);
        pr->setName(p_name);
        presences.insert(p_name, pr);
        appendChild(pr);
    } else {
        Presence* pr = itr.value();
        for (QMap<QString, QVariant>::const_iterator itr = data.begin(), end = data.end(); itr != end; ++itr) {
            pr->update(itr.key(), itr.value());
        }
    }
}

void Models::Contact::removePresence(const QString& name)
{
    QMap<QString, Presence*>::iterator itr = presences.find(name);
    
    if (itr == presences.end()) {
        qDebug() << "an attempt to remove non existing presence " << name << " from the contact " << jid << " of account " << getAccountName() << ", skipping";
    } else {
        Presence* pr = itr.value();
        presences.erase(itr);
        removeChild(pr->row());
        pr->deleteLater();
    }
}

void Models::Contact::refresh()
{
    QDateTime lastActivity;
    Presence* presence = 0;
    for (QMap<QString, Presence*>::iterator itr = presences.begin(), end = presences.end(); itr != end; ++itr) {
        Presence* pr = itr.value();
        QDateTime la = pr->getLastActivity();
        
        if (la > lastActivity) {
            lastActivity = la;
            presence = pr;
        }
    }
    
    if (presence != 0) {
        setAvailability(presence->getAvailability());
        setStatus(presence->getStatus());
    } else {
        setAvailability(Shared::Availability::offline);
        setStatus("");
    }
}

void Models::Contact::_removeChild(int index)
{
    Item* child = childItems[index];
    disconnect(child, &Item::childChanged, this, &Contact::refresh);
    Item::_removeChild(index);
    refresh();
}

void Models::Contact::_appendChild(Models::Item* child)
{
    Item::_appendChild(child);
    connect(child, &Item::childChanged, this, &Contact::refresh);
    refresh();
}

Shared::SubscriptionState Models::Contact::getState() const
{
    return state;
}

void Models::Contact::setState(Shared::SubscriptionState p_state)
{
    if (state != p_state) {
        state = p_state;
        changed(2);
    }
}

QIcon Models::Contact::getStatusIcon(bool big) const
{
    if (getMessagesCount() > 0) {
        return Shared::icon("mail-message", big);
    } else if (state == Shared::SubscriptionState::both || state == Shared::SubscriptionState::to) {
        return Shared::availabilityIcon(availability, big);;
    } else {
        return Shared::subscriptionStateIcon(state, big);
    }
}

void Models::Contact::toOfflineState()
{
    std::deque<Item*>::size_type size = childItems.size();
    if (size > 0) {
        emit childIsAboutToBeRemoved(this, 0, size - 1);
        for (std::deque<Item*>::size_type i = 0; i < size; ++i) {
            Item* item = childItems[0];
            disconnect(item, &Item::childChanged, this, &Contact::refresh);
            Item::_removeChild(0);
            item->deleteLater();
        }
        childItems.clear();
        presences.clear();
        emit childRemoved();
        refresh();
    }
}

QString Models::Contact::getDisplayedName() const
{
    return getContactName();
}

