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
                } catch (const Archive::NotFound& e) {
                    requestCache.emplace_back(requestedCount, before);
                    requestedCount = -1;
                    emit needHistory(archive->oldestId(), "");
                } catch (const Archive::Empty& e) {
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
            } catch (const Archive::NotFound& e) {
                qDebug("requesting id hasn't been found in archive, skipping");
            } catch (const Archive::Empty& e) {
                qDebug("requesting id hasn't been found in archive, skipping");
            }
            nextRequest();
            break;
    }
}

void Core::RosterItem::appendMessageToArchive(const Shared::Message& msg)
{
    const QString& id = msg.getId(); 
    if (id.size() > 0) {
        if (msg.storable()) {
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

void Core::RosterItem::requestFromEmpty(int count, const QString& before)
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
    QString path(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
    path += "/" + account + "/" + jid + "/" + (resource.size() == 0 ? jid : resource);
    return path;
}

bool Core::RosterItem::setAvatar(const QByteArray& data, const QString& resource)
{
    bool result = archive->setAvatar(data, false, resource);
    if (resource.size() == 0 && result) {
        if (data.size() == 0) {
            emit avatarChanged(Shared::Avatar::empty, "");
        } else {
            QMimeDatabase db;
            QMimeType type = db.mimeTypeForData(data);
            QString ext = type.preferredSuffix();
            emit avatarChanged(Shared::Avatar::valid, avatarPath(resource) + "." + ext);
        }
    }
    return result;
}

bool Core::RosterItem::setAutoGeneratedAvatar(const QString& resource)
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
    bool result = archive->setAvatar(arr, true, resource);
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
    bool hasAvatar = readAvatarInfo(info, resource);
    
    QByteArray ava = card.photo();
    Shared::VCard vCard;
    initializeVCard(vCard, card);
    Shared::Avatar type = Shared::Avatar::empty;
    QString path = "";
    
    if (ava.size() > 0) {
        bool changed = setAvatar(ava, resource);
        if (changed) {
            type = Shared::Avatar::valid;
            QMimeDatabase db;
            QMimeType type = db.mimeTypeForData(ava);
            QString ext = type.preferredSuffix();
            path = avatarPath(resource) + "." + ext;
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
            type = Shared::Avatar::autocreated;
            path = avatarPath(resource) + ".png";
        }
    }
    
    
    vCard.setAvatarType(type);
    vCard.setAvatarPath(path);
    
    if (resource.size() == 0) {
        emit avatarChanged(vCard.getAvatarType(), vCard.getAvatarPath());
    }
    
    return vCard;
}

