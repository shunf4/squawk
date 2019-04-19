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

#include "contact.h"

Core::Contact::Contact(const QString& pJid, const QString& account, QObject* parent):
    QObject(parent),
    jid(pJid),
    name(),
    groups(),
    archiveState(empty),
    archive(new Archive(jid)),
    subscriptionState(Shared::unknown)
{
    archive->open(account);
}

Core::Contact::~Contact()
{
    delete archive;
}

Core::Contact::ArchiveState Core::Contact::getArchiveState() const
{
    return archiveState;
}

QSet<QString> Core::Contact::getGroups() const
{
    return groups;
}

QString Core::Contact::getName() const
{
    return name;
}

void Core::Contact::setName(const QString& n)
{
    if (name != n) {
        name = n;
        emit nameChanged(name);
    }
}

Shared::SubscriptionState Core::Contact::getSubscriptionState() const
{
    return subscriptionState;
}

void Core::Contact::setGroups(const QSet<QString>& set)
{
    QSet<QString> toRemove = groups - set;
    QSet<QString> toAdd = set - groups;
    
    groups = set;
    
    for (QSet<QString>::iterator itr = toRemove.begin(), end = toRemove.end(); itr != end; ++itr) {
        emit groupRemoved(*itr);
    }
    
    for (QSet<QString>::iterator itr = toAdd.begin(), end = toAdd.end(); itr != end; ++itr) {
        emit groupAdded(*itr);
    }
}

void Core::Contact::setSubscriptionState(Shared::SubscriptionState state)
{
    if (subscriptionState != state) {
        subscriptionState = state;
        emit subscriptionStateChanged(subscriptionState);
    }
}

unsigned int Core::Contact::groupsCount() const
{
    return groups.size();
}
