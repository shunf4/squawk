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
    requestCache(),
    responseCache()
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
        performRequest(count, before);
    }
}

void Core::Contact::nextRequest()
{
    if (requestCache.size() > 0) {
        std::pair<int, QString> request = requestCache.front();
        requestCache.pop_front();
        performRequest(request.first, request.second);
    } else {
        syncronizing = false;
        requestedCount = 0;
        receivedCount = 0;
        hisoryCache.clear();
        responseCache.clear();
    }
}

void Core::Contact::performRequest(int count, const QString& before)
{
    syncronizing = true;
    requestedCount = count;
    receivedCount = 0;
    hisoryCache.clear();
    responseCache.clear();
    
    switch (archiveState) {
        case empty:
            emit needEarlierHistory(before, "", QDateTime(), QDateTime());
            break;
        case beginning: {
                bool found = false;
                if (appendCache.size() != 0) {
                    for (std::list<Shared::Message>::const_reverse_iterator itr = appendCache.rbegin(), end = appendCache.rend(); itr != end; ++itr) {
                        const Shared::Message& msg = *itr;
                        if (found) {
                            responseCache.emplace_front(msg);
                            ++receivedCount;
                        } else {
                            if (msg.getId() == before) {
                                found = true;
                                responseCache.emplace_front(*itr);
                                ++receivedCount;
                            }
                        }
                        if (responseCache.size() == count) {
                            break;
                        }
                    }
                    if (responseCache.size() == count) {
                        emit historyResponse(responseCache);
                        nextRequest();
                        break;
                    }
                }
                if (found) {
                    emit needEarlierHistory(responseCache.front().getId(), "", QDateTime(), QDateTime());
                } else {
                    if (requiestFromArchive(before)) {
                        nextRequest();
                    }
                }
            }
            break;
        case end: {
                std::list<Shared::Message> arc;
                if (count != -1) {
                    try {
                        arc = archive->getBefore(requestedCount - receivedCount, before);
                        responseCache.insert(responseCache.begin(), arc.begin(), arc.end());
                        emit historyResponse(responseCache);
                        nextRequest();
                    } catch (Archive::NotFound e) {
                        requestCache.emplace_back(count, before);
                        requestedCount = -1;
                        emit needEarlierHistory(archive->oldestId(), "", QDateTime(), QDateTime());
                    }
                } else {
                    try {
                        arc = archive->getBefore(1, before);
                        //just do nothing since response is not required
                        nextRequest();      //may be even it's a signal that the history is now complete?
                    } catch (Archive::NotFound e) {
                        emit needEarlierHistory(archive->oldestId(), "", QDateTime(), QDateTime());
                    }
                }
            }
            break;
        case chunk:
            //from last
            break;
        case complete:
            //just give
            break;
    }
}

bool Core::Contact::requiestFromArchive(const QString& before)
{
    std::list<Shared::Message> arc;
    QString lBefore;
    if (responseCache.size() > 0) {
        lBefore = responseCache.front().getId();
    } else {
        lBefore = before;
    }
    if (requestedCount != -1) {
        try {
            arc = archive->getBefore(requestedCount - receivedCount, lBefore);
            responseCache.insert(responseCache.begin(), arc.begin(), arc.end());
            emit historyResponse(responseCache);
            return true;
        } catch (Archive::NotFound e) {
            requestCache.emplace_back(requestedCount, before);
            requestedCount = -1;
            switch (archiveState) {
                case empty:
                case beginning:
                case end:
                case chunk:
                case complete:
            }
            emit needEarlierHistory("", archive->newestId(), QDateTime(), QDateTime());
            return false;
        }
    } else {
        try {
            arc = archive->getBefore(1, lBefore);
            //just do nothing since response is not required
            //may be even it's a signal that the history is now complete?
            return true;
        } catch (Archive::NotFound e) {
            emit needEarlierHistory("", archive->newestId(), QDateTime(), QDateTime());
            return false;
        }
    }
}


void Core::Contact::appendMessageToArchive(const Shared::Message& msg)
{
    if (msg.getId().size() > 0 && msg.getBody().size() > 0) {
        switch (archiveState) {
            case empty:
                if (archive->addElement(msg)) {
                    archiveState = end;
                };
                requestHistory(-1, msg.getId());
                break;
            case beginning:
                appendCache.emplace_back(msg);
                requestHistory(-1, msg.getId());
                break;
            case end:
                archive->addElement(msg);
                break;
            case chunk:
                appendCache.emplace_back(msg);
                requestHistory(-1, msg.getId());
                break;
            case complete:
                archive->addElement(msg);
                break;
        }
        
    }
}
