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

#include "rosteritem.h"
#include "account.h"

#include <QDebug>

Core::RosterItem::RosterItem(const QString& pJid, const QString& pAccount, QObject* parent):
    QObject(parent),
    jid(pJid),
    account(pAccount),
    name(),
    archiveState(empty),
    archive(new Archive(jid)),
    syncronizing(false),
    requestedCount(0),
    requestedBefore(),
    hisoryCache(),
    appendCache(),
    responseCache(),
    requestCache(),
    toCorrect(),
    muc(false)
{
    archive->open(account);
    
    if (archive->size() != 0) {
        if (archive->isFromTheBeginning()) {
            archiveState = beginning;
        } else {
            archiveState = chunk;
        }
    }
}

Core::RosterItem::~RosterItem()
{
    delete archive;
}

Core::RosterItem::ArchiveState Core::RosterItem::getArchiveState() const
{
    return archiveState;
}

QString Core::RosterItem::getName() const
{
    return name;
}

void Core::RosterItem::setName(const QString& n)
{
    if (name != n) {
        name = n;
        emit nameChanged(name);
    }
}

void Core::RosterItem::addMessageToArchive(const Shared::Message& msg)
{
    if (msg.storable()) {
        hisoryCache.push_back(msg);
        std::map<QString, Shared::Message>::iterator itr = toCorrect.find(msg.getId());
        if (itr != toCorrect.end()) {
            hisoryCache.back().change({
                {"body", itr->second.getBody()},
                {"stamp", itr->second.getTime()}
            });
            toCorrect.erase(itr);
        }
    }
}

void Core::RosterItem::correctMessageInArchive(const QString& originalId, const Shared::Message& msg)
{
    if (msg.storable()) {
        QDateTime thisTime = msg.getTime();
        std::map<QString, Shared::Message>::iterator itr = toCorrect.find(originalId);
        if (itr != toCorrect.end()) {
            if (itr->second.getTime() < thisTime) {
                itr->second = msg;
            }
            return;
        }
        
        bool found = changeMessage(originalId, {
            {"body", msg.getBody()},
            {"stamp", thisTime}
        });
        if (!found) {
            toCorrect.insert(std::make_pair(originalId, msg));
        }
    }
}

void Core::RosterItem::requestHistory(int count, const QString& before)
{
    if (syncronizing) {
        requestCache.emplace_back(count, before);
    } else {
        performRequest(count, before);
    }
}

void Core::RosterItem::nextRequest()
{
    if (syncronizing) {
        if (requestedCount != -1) {
            bool last = false;
            if (archiveState == beginning || archiveState == complete) {
                QString firstId = archive->oldestId();
                if (responseCache.size() == 0) {
                    if (requestedBefore == firstId) {
                        last = true;
                    }
                } else {
                    if (responseCache.front().getId() == firstId) {
                        last = true;
                    }
                }
            } else if (archiveState == empty && responseCache.size() == 0) {
                last = true;
            }
            emit historyResponse(responseCache, last);
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

void Core::RosterItem::performRequest(int count, const QString& before)
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
            emit needHistory("", getId(msg), msg.getTime());
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
                } catch (const Archive::NotFound& e) {
                    requestCache.emplace_back(requestedCount, before);
                    requestedCount = -1;
                    emit needHistory(getId(archive->oldest()), "");
                } catch (const Archive::Empty& e) {
                    requestCache.emplace_back(requestedCount, before);
                    requestedCount = -1;
                    emit needHistory(getId(archive->oldest()), "");
                }
                
                if (found) {
                    int rSize = responseCache.size();
                    if (rSize < count) {
                        if (rSize != 0) {
                            emit needHistory(getId(responseCache.front()), "");
                        } else {
                            QString bf;
                            if (muc) {
                                bf = archive->stanzaIdById(before);
                                if (bf.size() < 0) {
                                    qDebug() << "Didn't find stanzaId for id requesting history for" << jid << ", falling back to requesting by id";
                                    bf = before;
                                }
                            } else {
                                bf = before;
                            }
                            emit needHistory(bf, "");
                        }
                    } else {
                        nextRequest();
                    }
                }
            } else {
                emit needHistory(getId(archive->oldest()), "");
            }
            break;
        case complete:
            try {
                std::list<Shared::Message> arc = archive->getBefore(requestedCount - responseCache.size(), before);
                responseCache.insert(responseCache.begin(), arc.begin(), arc.end());
            } catch (const Archive::NotFound& e) {
                qDebug("requesting id hasn't been found in archive, skipping");
            } catch (const Archive::Empty& e) {
                qDebug("requesting id hasn't been found in archive, skipping");
            }
            nextRequest();
            break;
    }
}

QString Core::RosterItem::getId(const Shared::Message& msg)
{
    QString id;
    if (muc) {
        id = msg.getStanzaId();
    } else {
        id = msg.getId();
    }
    return id;
}

void Core::RosterItem::appendMessageToArchive(const Shared::Message& msg)
{
    if (msg.getId().size() > 0) {
        if (msg.storable()) {
            switch (archiveState) {
                case empty:
                    if (archive->addElement(msg)) {
                        archiveState = end;
                    }
                    if (!syncronizing) {
                        requestHistory(-1, getId(msg));
                    }
                    break;
                case beginning:
                    if (!archive->hasElement(msg.getId())) {
                        appendCache.push_back(msg);
                        if (!syncronizing) {
                            requestHistory(-1, getId(msg));
                        }
                    }
                    break;
                case end:
                    archive->addElement(msg);
                    break;
                case chunk:
                    if (!archive->hasElement(msg.getId())) {
                        appendCache.push_back(msg);
                        if (!syncronizing) {
                            requestHistory(-1, getId(msg));
                        }
                    }
                    break;
                case complete:
                    archive->addElement(msg);
                    break;
            }
        } else if (!syncronizing && archiveState == empty) {
            requestHistory(-1, getId(msg));
        }
    }
}

bool Core::RosterItem::changeMessage(const QString& id, const QMap<QString, QVariant>& data)
{
    bool found = false;
    for (Shared::Message& msg : appendCache) {
        if (msg.getId() == id) {
            msg.change(data);
            found = true;
            break;
        }
    }
    
    if (!found) {
        for (Shared::Message& msg : hisoryCache) {
            if (msg.getId() == id) {
                msg.change(data);
                found = true;
                break;
            }
        }
    }
    
    if (!found) {
        try {
            archive->changeMessage(id, data);
            found = true;
        } catch (const Archive::NotFound& e) {
            qDebug() << "An attempt to change state to the message" << id << "but it couldn't be found";
        }
    }
    
    if (found) {
        for (Shared::Message& msg : responseCache) {
            if (msg.getId() == id) {
                msg.change(data);
                break;
            }
        }
    }
    
    return found;
}

void Core::RosterItem::flushMessagesToArchive(bool finished, const QString& firstId, const QString& lastId)
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
                added += archive->addElements(appendCache);
                appendCache.clear();
                nextRequest();
            } else {
                emit needHistory("", lastId);
            }
            break;
        case chunk:
            if (finished) {
                archiveState = end;
                added += archive->addElements(appendCache);
                appendCache.clear();
                nextRequest();
            } else {
                emit needHistory("", lastId);
            }
            break;
        case empty:
            wasEmpty = true;
            archiveState = end;
            [[fallthrough]];
        case end:
            added += archive->addElements(appendCache);
            appendCache.clear();
            if (finished && (added > 0 || !wasEmpty)) {
                archiveState = complete;
                archive->setFromTheBeginning(true);
            }
            if (added == 0 && wasEmpty) {
                archiveState = empty;
                nextRequest();
                break;
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
                } catch (const Archive::NotFound& e) {
                    
                } catch (const Archive::Empty& e) {
                    
                }
                if (!found || requestedCount > int(responseCache.size())) {
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

QString Core::RosterItem::getServer() const
{
    QStringList lst = jid.split("@");
    return lst.back();
}

bool Core::RosterItem::isMuc() const
{
    return muc;
}

QString Core::RosterItem::avatarPath(const QString& resource) const
{
    QString path = folderPath() + "/" + (resource.size() == 0 ? jid : resource);
    return path;
}

QString Core::RosterItem::folderPath() const
{
    QString path(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
    path += "/" + account + "/" + jid;
    return path;
}

bool Core::RosterItem::setAvatar(const QByteArray& data, Archive::AvatarInfo& info, const QString& resource)
{
    bool result = archive->setAvatar(data, info, false, resource);
    if (resource.size() == 0 && result) {
        if (data.size() == 0) {
            emit avatarChanged(Shared::Avatar::empty, "");
        } else {
            emit avatarChanged(Shared::Avatar::valid, avatarPath(resource) + "." + info.type);
        }
    }
    return result;
}

bool Core::RosterItem::setAutoGeneratedAvatar(const QString& resource)
{
    Archive::AvatarInfo info;
    return setAutoGeneratedAvatar(info, resource);
}

bool Core::RosterItem::setAutoGeneratedAvatar(Archive::AvatarInfo& info, const QString& resource)
{
    QImage image(96, 96, QImage::Format_ARGB32_Premultiplied);
    QPainter painter(&image);
    quint8 colorIndex = rand() % Shared::colorPalette.size();
    const QColor& bg = Shared::colorPalette[colorIndex];
    painter.fillRect(image.rect(), bg);
    QFont f;
    f.setBold(true);
    f.setPixelSize(72);
    painter.setFont(f);
    if (bg.lightnessF() > 0.5) {
        painter.setPen(Qt::black);
    } else {
        painter.setPen(Qt::white);
    }
    painter.drawText(image.rect(), Qt::AlignCenter | Qt::AlignVCenter, resource.size() == 0 ? jid.at(0).toUpper() : resource.at(0).toUpper());
    QByteArray arr;
    QBuffer stream(&arr);
    stream.open(QBuffer::WriteOnly);
    image.save(&stream, "PNG");
    stream.close();
    bool result = archive->setAvatar(arr, info, true, resource);
    if (resource.size() == 0 && result) {
        emit avatarChanged(Shared::Avatar::autocreated, avatarPath(resource) + ".png");
    }
    return result;
}

bool Core::RosterItem::readAvatarInfo(Archive::AvatarInfo& target, const QString& resource) const
{
    return archive->readAvatarInfo(target, resource);
}

Shared::VCard Core::RosterItem::handleResponseVCard(const QXmppVCardIq& card, const QString& resource)
{
    Archive::AvatarInfo info;
    Archive::AvatarInfo newInfo;
    bool hasAvatar = readAvatarInfo(info, resource);
    
    QByteArray ava = card.photo();
    Shared::VCard vCard;
    initializeVCard(vCard, card);
    Shared::Avatar type = Shared::Avatar::empty;
    QString path = "";
    
    if (ava.size() > 0) {
        bool changed = setAvatar(ava, newInfo, resource);
        if (changed) {
            type = Shared::Avatar::valid;
            path = avatarPath(resource) + "." + newInfo.type;
        } else if (hasAvatar) {
            if (info.autogenerated) {
                type = Shared::Avatar::autocreated;
                path = avatarPath(resource) + ".png";
            } else {
                type = Shared::Avatar::valid;
                path = avatarPath(resource) + "." + info.type;
            }
        }
    } else {
        if (!hasAvatar || !info.autogenerated) {
            setAutoGeneratedAvatar(resource);
        }
        type = Shared::Avatar::autocreated;
        path = avatarPath(resource) + ".png";
    }
    
    vCard.setAvatarType(type);
    vCard.setAvatarPath(path);
    
    if (resource.size() == 0) {
        emit avatarChanged(vCard.getAvatarType(), vCard.getAvatarPath());
    }
    
    return vCard;
}

void Core::RosterItem::clearArchiveRequests()
{
    syncronizing = false;
    requestedCount = 0;
    requestedBefore = "";
    for (const std::pair<int, QString>& pair : requestCache) {
        if (pair.first != -1) {
            emit historyResponse(responseCache, false);        //just to notify those who still waits with whatever happened to be left in caches yet
        }
        responseCache.clear();
    }
    hisoryCache.clear();
    responseCache.clear();  //in case the cycle never runned
    appendCache.clear();
    requestCache.clear();
}

void Core::RosterItem::downgradeDatabaseState()
{
    if (archiveState == ArchiveState::complete) {
        archiveState = ArchiveState::beginning;
    }
    
    if (archiveState == ArchiveState::end) {
        archiveState = ArchiveState::chunk;
    }
}

Shared::Message Core::RosterItem::getMessage(const QString& id)
{
    for (const Shared::Message& msg : appendCache) {
        if (msg.getId() == id) {
            return msg;
        }
    }
    
    for (Shared::Message& msg : hisoryCache) {
        if (msg.getId() == id) {
            return msg;
        }
    }
    
    return archive->getElement(id);
}
