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
    subscriptionState(Shared::unknown),
    syncronizing(false),
    requestedCount(0),
    receivedCount(0),
    hisoryCache(),
    appendCache(),
    requestCache()
{
    archive->open(account);
    if (archive->isFromTheBeginning()) {
        archiveState = beginning;
    } else {
        if (archive->size() != 0) {
            archiveState = chunk;
        }
    }
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

void Core::Contact::addMessageToArchive(const Shared::Message& msg)
{
    
}

void Core::Contact::requestHistory(int count, const QString& before)
{
    if (syncronizing) {
        requestCache.emplace_back(count, before);
    } else {
        switch (archiveState) {
            case empty:
                if (appendCache.size() != 0) {
                    //from last
                } else {
                    //search
                }
                break;
            case beginning:
                //from last
                break;
            case end:
                //try
                break;
            case chunk:
                //from last
                break;
            case complete:
                //just give
                break;
        }
        syncronizing = true;
        requestedCount = count;
        receivedCount = 0;
        hisoryCache.clear();
    }
}

