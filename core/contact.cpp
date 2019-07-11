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
#include <QDebug>

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
    requestedBefore(),
    hisoryCache(),
    appendCache(),
    responseCache(),
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
    if (msg.getId().size() > 0 && msg.getBody().size() > 0) {
        hisoryCache.push_back(msg);
    }
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
    if (syncronizing) {
        if (requestedCount != -1) {
            emit historyResponse(responseCache);
        }
    }
    if (requestCache.size() > 0) {
        std::pair<int, QString> request = requestCache.front();
        requestCache.pop_front();
        performRequest(request.first, request.second);
    } else {
        syncronizing = false;
        requestedCount = 0;
        requestedBefore = "";
        hisoryCache.clear();
        responseCache.clear();
    }
}

void Core::Contact::performRequest(int count, const QString& before)
{
    syncronizing = true;
    requestedCount = count;
    requestedBefore = before;
    hisoryCache.clear();
    responseCache.clear();
    
    switch (archiveState) {
        case empty:
            emit needHistory(before, "");
            break;
        case chunk:
        case beginning: {
            if (count != -1) {
                requestCache.emplace_back(requestedCount, before);
                requestedCount = -1;
            }
            Shared::Message msg = archive->newest();
            emit needHistory("", msg.getId(), msg.getTime());
        }
            break;
        case end: 
            if (count != -1) {
                QString lBefore;
                if (responseCache.size() > 0) {
                    lBefore = responseCache.front().getId();
                } else {
                    lBefore = before;
                }
                bool found = false;
                try {
                    std::list<Shared::Message> arc = archive->getBefore(requestedCount - responseCache.size(), lBefore);
                    responseCache.insert(responseCache.begin(), arc.begin(), arc.end());
                    found = true;
                } catch (Archive::NotFound e) {
                    requestCache.emplace_back(requestedCount, before);
                    requestedCount = -1;
                    emit needHistory(archive->oldestId(), "");
                } catch (Archive::Empty e) {
                    requestCache.emplace_back(requestedCount, before);
                    requestedCount = -1;
                    emit needHistory(archive->oldestId(), "");
                }
                
                if (found) {
                    int rSize = responseCache.size();
                    if (rSize < count) {
                        if (rSize != 0) {
                            emit needHistory(responseCache.front().getId(), "");
                        } else {
                            emit needHistory(before, "");
                        }
                    } else {
                        nextRequest();
                    }
                }
            } else {
                emit needHistory(archive->oldestId(), "");
            }
            break;
        case complete:
            try {
                std::list<Shared::Message> arc = archive->getBefore(requestedCount - responseCache.size(), before);
                responseCache.insert(responseCache.begin(), arc.begin(), arc.end());
            } catch (Archive::NotFound e) {
                qDebug("requesting id hasn't been found in archive, skipping");
            } catch (Archive::Empty e) {
                qDebug("requesting id hasn't been found in archive, skipping");
            }
            nextRequest();
            break;
    }
}

void Core::Contact::appendMessageToArchive(const Shared::Message& msg)
{
    const QString& id = msg.getId(); 
    if (id.size() > 0) {
        if (msg.getBody().size() > 0) {
            switch (archiveState) {
                case empty:
                    if (archive->addElement(msg)) {
                        archiveState = end;
                    }
                    if (!syncronizing) {
                        requestHistory(-1, id);
                    }
                    break;
                case beginning:
                    appendCache.push_back(msg);
                    if (!syncronizing) {
                        requestHistory(-1, id);
                    }
                    break;
                case end:
                    archive->addElement(msg);
                    break;
                case chunk:
                    appendCache.push_back(msg);
                    if (!syncronizing) {
                        requestHistory(-1, id);
                    }
                    break;
                case complete:
                    archive->addElement(msg);
                    break;
            }
        } else if (!syncronizing && archiveState == empty) {
            requestHistory(-1, id);
        }
    }
}

void Core::Contact::flushMessagesToArchive(bool finished, const QString& firstId, const QString& lastId)
{
    unsigned int added(0);
    if (hisoryCache.size() > 0) {
        added = archive->addElements(hisoryCache);
        qDebug() << "Added" << added << "messages to the archive";
        hisoryCache.clear();
    }
    
    bool wasEmpty = false;
    switch (archiveState) {
        case beginning:
            if (finished) {
                archiveState = complete;
                archive->addElements(appendCache);
                appendCache.clear();
                nextRequest();
            } else {
                emit needHistory("", lastId);
            }
            break;
        case chunk:
            if (finished) {
                archiveState = end;
                archive->addElements(appendCache);
                appendCache.clear();
                nextRequest();
            } else {
                emit needHistory("", lastId);
            }
            break;
        case empty:
            wasEmpty = true;
            archiveState = end;
        case end:
            if (finished && (added > 0 || !wasEmpty)) {
                archiveState = complete;
                archive->setFromTheBeginning(true);
            }
            if (requestedCount != -1) {
                QString before;
                if (responseCache.size() > 0) {
                    before = responseCache.front().getId();
                } else {
                    before = requestedBefore;
                }
                
                bool found = false;
                try {
                    std::list<Shared::Message> arc = archive->getBefore(requestedCount - responseCache.size(), before);
                    responseCache.insert(responseCache.begin(), arc.begin(), arc.end());
                    found = true;
                } catch (Archive::NotFound e) {
                    
                } catch (Archive::Empty e) {
                    
                }
                if (!found || requestedCount > responseCache.size()) {
                    if (archiveState == complete) {
                        nextRequest();
                    } else {
                        emit needHistory(firstId, "");
                    }
                } else {
                    nextRequest();
                }
            } else {
                if (added != 0) {
                    nextRequest();
                } else {
                    emit needHistory(firstId, "");
                }
            }
            break;
        case complete:
            nextRequest();
            break;
    }
}

void Core::Contact::requestFromEmpty(int count, const QString& before)
{
    if (syncronizing) {
        qDebug("perform from empty didn't work, another request queued");
    } else {
        if (archiveState != empty) {
            qDebug("perform from empty didn't work, the state is not empty");
            requestHistory(count, before);
        } else {
            syncronizing = true;
            requestedCount = count;
            requestedBefore = "";
            hisoryCache.clear();
            responseCache.clear();
            
            emit needHistory(before, "");
        }
    }
}

