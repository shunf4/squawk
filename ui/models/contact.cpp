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

Models::Contact::Contact(const QString& p_jid ,const QMap<QString, QVariant> &data, Item *parentItem):
    Item(Item::contact, data, parentItem),
    jid(p_jid),
    availability(Shared::offline),
    state(Shared::none),
    presences(),
    messages(),
    childMessages(0)
{
    QMap<QString, QVariant>::const_iterator itr = data.find("state");
    if (itr != data.end()) {
        setState(itr.value().toUInt());
    }
}

Models::Contact::~Contact()
{
}

QString Models::Contact::getJid() const
{
    return jid;
}

void Models::Contact::setJid(const QString p_jid)
{
    if (jid != p_jid) {
        jid = p_jid;
        changed(1);
    }
}

void Models::Contact::setAvailability(unsigned int p_state)
{
    if (p_state <= Shared::availabilityHighest) {
        Shared::Availability state = static_cast<Shared::Availability>(p_state);
        setAvailability(state);
    } else {
        qDebug() << "An attempt to set invalid availability " << p_state << " to the contact " << jid;
    }
}

void Models::Contact::setState(unsigned int p_state)
{
    if (p_state <= Shared::subscriptionStateHighest) {
        Shared::SubscriptionState state = static_cast<Shared::SubscriptionState>(p_state);
        setState(state);
    } else {
        qDebug() << "An attempt to set invalid subscription state " << p_state << " to the contact " << jid;
    }
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
    return 6;
}

QVariant Models::Contact::data(int column) const
{
    switch (column) {
        case 0:
            return getContactName();
        case 1:
            return jid;
        case 2:
            return state;
        case 3:
            return availability;
        case 4:
            return getMessagesCount();
        case 5:
            return getStatus();
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
    } else if (field == "jid") {
        setJid(value.toString());
    } else if (field == "availability") {
        setAvailability(value.toUInt());
    } else if (field == "state") {
        setState(value.toUInt());
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
    unsigned int count = 0;
    for (QMap<QString, Presence*>::iterator itr = presences.begin(), end = presences.end(); itr != end; ++itr) {
        Presence* pr = itr.value();
        QDateTime la = pr->getLastActivity();
        count += pr->getMessagesCount();
        
        if (la > lastActivity) {
            lastActivity = la;
            presence = pr;
        }
    }
    
    if (presence != 0) {
        setAvailability(presence->getAvailability());
        setStatus(presence->getStatus());
    } else {
        setAvailability(Shared::offline);
        setStatus("");
    }
    
    if (childMessages != count) {
        childMessages = count;
        changed(4);
    }
}

void Models::Contact::_removeChild(int index)
{
    Item* child = childItems[index];
    disconnect(child, SIGNAL(childChanged(Models::Item*, int, int)), this, SLOT(refresh()));
    Item::_removeChild(index);
    refresh();
}

void Models::Contact::appendChild(Models::Item* child)
{
    Item::appendChild(child);
    connect(child, SIGNAL(childChanged(Models::Item*, int, int)), this, SLOT(refresh()));
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
    } else if (state == Shared::both) {
        return Shared::availabilityIcon(availability, big);;
    } else {
        return Shared::subscriptionStateIcon(state, big);
    }
}

void Models::Contact::addMessage(const Shared::Message& data)
{
    const QString& res = data.getPenPalResource();
    if (res.size() > 0) {
        QMap<QString, Presence*>::iterator itr = presences.find(res);
        if (itr == presences.end()) {
            // this is actually the place when I can spot someone's invisible presence, and there is nothing criminal in it, cuz the sender sent us a message
            // therefore he have revealed himself
            // the only issue is to find out when the sender is gone offline
            Presence* pr = new Presence({});
            pr->setName(res);
            pr->setAvailability(Shared::invisible);
            pr->setLastActivity(QDateTime::currentDateTime());
            presences.insert(res, pr);
            appendChild(pr);
            pr->addMessage(data);
            return;
        }
        itr.value()->addMessage(data);
    } else {
        messages.emplace_back(data);
        changed(4);
    }
}

unsigned int Models::Contact::getMessagesCount() const
{
    return messages.size() + childMessages;
}

void Models::Contact::dropMessages()
{
    if (messages.size() > 0) {
        messages.clear();
        changed(4);
    }
    
    for (QMap<QString, Presence*>::iterator itr = presences.begin(), end = presences.end(); itr != end; ++itr) {
        itr.value()->dropMessages();
    }
}

void Models::Contact::getMessages(Models::Contact::Messages& container) const
{
    for (Messages::const_iterator itr = messages.begin(), end = messages.end(); itr != end; ++itr) {
        const Shared::Message& msg = *itr;
        container.push_back(msg);
    }
    
    for (QMap<QString, Presence*>::const_iterator itr = presences.begin(), end = presences.end(); itr != end; ++itr) {
        itr.value()->getMessages(container);
    }
}

void Models::Contact::toOfflineState()
{
    emit childIsAboutToBeRemoved(this, 0, childItems.size());
    for (int i = 0; i < childItems.size(); ++i) {
        Item* item = childItems[i];
        disconnect(item, SIGNAL(childChanged(Models::Item*, int, int)), this, SLOT(refresh()));
        Item::_removeChild(i);
        item->deleteLater();
    }
    childItems.clear();
    presences.clear();
    emit childRemoved();
    refresh();
}

QString Models::Contact::getDisplayedName() const
{
    return getContactName();
}

bool Models::Contact::columnInvolvedInDisplay(int col)
{
    return Item::columnInvolvedInDisplay(col) && col == 1;
}
