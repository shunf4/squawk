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

#include "messagefeed.h"
#include "element.h"
#include "room.h"

#include <QDebug>

const QHash<int, QByteArray> Models::MessageFeed::roles = {
    {Text, "text"},
    {Sender, "sender"},
    {Date, "date"},
    {DeliveryState, "deliveryState"},
    {Correction, "correction"},
    {SentByMe,"sentByMe"},
    {Avatar, "avatar"},
    {Attach, "attach"},
    {Id, "id"},
    {Bulk, "bulk"}
};

Models::MessageFeed::MessageFeed(const Element* ri, QObject* parent):
    QAbstractListModel(parent),
    storage(),
    indexById(storage.get<id>()),
    indexByTime(storage.get<time>()),
    rosterItem(ri),
    syncState(incomplete),
    uploads(),
    downloads()
{
}

Models::MessageFeed::~MessageFeed()
{
    for (Shared::Message* message : storage) {
        delete message;
    }
}

void Models::MessageFeed::addMessage(const Shared::Message& msg)
{
    QString id = msg.getId();
    StorageById::const_iterator itr = indexById.find(id);
    if (itr != indexById.end()) {
        qDebug() << "received more then one message with the same id, skipping yet the new one";
        return;
    }
    
    Shared::Message* copy = new Shared::Message(msg);
    StorageByTime::const_iterator tItr = indexByTime.upper_bound(msg.getTime());
    int position;
    if (tItr == indexByTime.end()) {
        position = storage.size();
    } else {
        position = indexByTime.rank(tItr);
    }
    beginInsertRows(QModelIndex(), position, position);
    storage.insert(copy);
    endInsertRows();
}

void Models::MessageFeed::changeMessage(const QString& id, const QMap<QString, QVariant>& data)
{
    StorageById::iterator itr = indexById.find(id);
    if (itr == indexById.end()) {
        qDebug() << "received a command to change a message, but the message couldn't be found, skipping";
        return;
    }
    
    Shared::Message* msg = *itr;
    QModelIndex index = modelIndexByTime(id, msg->getTime());
    Shared::Message::Change functor(data);
    bool success = indexById.modify(itr, functor);
    if (!success) {
        qDebug() << "received a command to change a message, but something went wrong modifying message in the feed, throwing error";
        throw 872;
    }
    
    if (functor.hasIdBeenModified()) {
        
    }
    
    //change message is a final event in download/upload event train
    //only after changeMessage we can consider the download is done
    Progress::const_iterator dItr = downloads.find(id);
    if (dItr != downloads.end()) {
        if (dItr->second == 1) {
            downloads.erase(dItr);
        }
    } else {
        dItr = uploads.find(id);
        if (dItr != uploads.end()) {
            if (dItr->second == 1) {
                uploads.erase(dItr);
            }
        }
    }
    
    emit dataChanged(index, index);
}

void Models::MessageFeed::removeMessage(const QString& id)
{
}

QVariant Models::MessageFeed::data(const QModelIndex& index, int role) const
{
    int i = index.row();
    QVariant answer;
    
    StorageByTime::const_iterator itr = indexByTime.nth(i);
    if (itr != indexByTime.end()) {
        const Shared::Message* msg = *itr;
        
        switch (role) {
            case Qt::DisplayRole:
            case Text: {
                QString body = msg->getBody();
                if (body != msg->getOutOfBandUrl()) {
                    answer = body;
                }
            }
                break;
            case Sender: 
                if (sentByMe(*msg)) {
                    answer = rosterItem->getAccountName();
                } else {
                    if (rosterItem->isRoom()) {
                        answer = msg->getFromResource();
                    } else {
                        answer = rosterItem->getDisplayedName();
                    }
                }
                break;
            case Date: 
                answer = msg->getTime();
                break;
            case DeliveryState: 
                answer = static_cast<unsigned int>(msg->getState());
                break;
            case Correction: 
                answer = msg->getEdited();
                break;
            case SentByMe: 
                answer = sentByMe(*msg);
                break;
            case Avatar: {
                QString path;
                if (sentByMe(*msg)) {
                    path = rosterItem->getAccountAvatarPath();
                } else if (!rosterItem->isRoom()) {
                    if (rosterItem->getAvatarState() != Shared::Avatar::empty) {
                        path = rosterItem->getAvatarPath();
                    }
                } else {
                    const Room* room = static_cast<const Room*>(rosterItem);
                    path = room->getParticipantIconPath(msg->getFromResource());
                }
                
                if (path.size() == 0) {
                    answer = Shared::iconPath("user", true);
                } else {
                    answer = path;
                }
            }
                break;
            case Attach: 
                answer.setValue(fillAttach(*msg));
                break;
            case Id: 
                answer.setValue(msg->getId());
                break;
            case Bulk: {
                FeedItem item;
                item.id = msg->getId();
                item.sentByMe = sentByMe(*msg);
                item.date = msg->getTime();
                item.state = msg->getState();
                item.correction = msg->getEdited();
                
                QString body = msg->getBody();
                if (body != msg->getOutOfBandUrl()) {
                    item.text = body;
                }
                
                item.avatar.clear();
                if (item.sentByMe) {
                    item.sender = rosterItem->getAccountName();
                    item.avatar = rosterItem->getAccountAvatarPath();
                } else {
                    if (rosterItem->isRoom()) {
                        item.sender = msg->getFromResource();
                        const Room* room = static_cast<const Room*>(rosterItem);
                        item.avatar = room->getParticipantIconPath(msg->getFromResource());
                    } else {
                        item.sender = rosterItem->getDisplayedName();
                        if (rosterItem->getAvatarState() != Shared::Avatar::empty) {
                            item.avatar = rosterItem->getAvatarPath();
                        }
                    }
                }
                
                if (item.avatar.size() == 0) {
                    item.avatar = Shared::iconPath("user", true);
                }
                item.attach = fillAttach(*msg);
                answer.setValue(item);
            }
                break;
            default:
                break;
        }
    }
    
    return answer;
}

int Models::MessageFeed::rowCount(const QModelIndex& parent) const
{
    return storage.size();
}

unsigned int Models::MessageFeed::unreadMessagesCount() const
{
    return storage.size(); //let's say they are all new for now =)
}

bool Models::MessageFeed::canFetchMore(const QModelIndex& parent) const
{
    return syncState == incomplete;
}

void Models::MessageFeed::fetchMore(const QModelIndex& parent)
{
    if (syncState == incomplete) {
        syncState = syncing;
        emit requestStateChange(true);
        
        if (storage.size() == 0) {
            emit requestArchive("");
        } else {
            emit requestArchive((*indexByTime.rbegin())->getId());
        }
    }
}

void Models::MessageFeed::responseArchive(const std::list<Shared::Message> list, bool last)
{
    Storage::size_type size = storage.size();
    
    beginInsertRows(QModelIndex(), size, size + list.size() - 1);
    for (const Shared::Message& msg : list) {
        Shared::Message* copy = new Shared::Message(msg);
        storage.insert(copy);
    }
    endInsertRows();
    
    if (syncState == syncing) {
        if (last) {
            syncState = complete;
        } else {
            syncState = incomplete;
        }
        emit requestStateChange(false);
    }
}

QHash<int, QByteArray> Models::MessageFeed::roleNames() const
{
    return roles;
}

bool Models::MessageFeed::sentByMe(const Shared::Message& msg) const
{
    if (rosterItem->isRoom()) {
        const Room* room = static_cast<const Room*>(rosterItem);
        return room->getNick().toLower() == msg.getFromResource().toLower();
    } else {
        return msg.getOutgoing();
    }
}

Models::Attachment Models::MessageFeed::fillAttach(const Shared::Message& msg) const
{
    ::Models::Attachment att;
    
    att.localPath = msg.getAttachPath();
    att.remotePath = msg.getOutOfBandUrl();
    
    if (att.remotePath.size() == 0) {
        if (att.localPath.size() == 0) {
            att.state = none;
        } else {
            Progress::const_iterator itr = uploads.find(msg.getId());
            if (itr == uploads.end()) {
                att.state = local;
            } else {
                att.state = uploading;
                att.progress = itr->second;
            }
        }
    } else {
        if (att.localPath.size() == 0) {
            Progress::const_iterator itr = downloads.find(msg.getId());
            if (itr == downloads.end()) {
                att.state = remote;
            } else {
                att.state = downloading;
                att.progress = itr->second;
            }
        } else {
            att.state = ready;
        }
    }
    
    return att;
}

void Models::MessageFeed::downloadAttachment(const QString& messageId)
{
    QModelIndex ind = modelIndexById(messageId);
    if (ind.isValid()) {
        std::pair<Progress::iterator, bool> progressPair = downloads.insert(std::make_pair(messageId, 0));
        if (progressPair.second) {     //Only to take action if we weren't already downloading it
            Shared::Message* msg = static_cast<Shared::Message*>(ind.internalPointer());
            emit dataChanged(ind, ind);
            emit fileDownloadRequest(msg->getOutOfBandUrl());
        } else {
            qDebug() << "Attachment download for message with id" << messageId << "is already in progress, skipping";
        }
    } else {
        qDebug() << "An attempt to download an attachment for the message that doesn't exist. ID:" << messageId;
    }
}

void Models::MessageFeed::uploadAttachment(const QString& messageId)
{
    qDebug() << "request to upload attachment of the message" << messageId;
}

void Models::MessageFeed::fileProgress(const QString& messageId, qreal value, bool up)
{
    Progress* pr = 0;
    if (up) {
        pr = &uploads;
    } else {
        pr = &downloads;
    }
    
    Progress::iterator itr = pr->find(messageId);
    if (itr != pr->end()) {
        itr->second = value;
        QModelIndex ind = modelIndexById(messageId);
        emit dataChanged(ind, ind);
    }
}

void Models::MessageFeed::fileComplete(const QString& messageId, bool up)
{
    fileProgress(messageId, 1, up);
}

void Models::MessageFeed::fileError(const QString& messageId, const QString& error, bool up)
{
    //TODO
}


QModelIndex Models::MessageFeed::modelIndexById(const QString& id) const
{
    StorageById::const_iterator itr = indexById.find(id);
    if (itr != indexById.end()) {
        Shared::Message* msg = *itr;
        return modelIndexByTime(id, msg->getTime());
    }
    
    return QModelIndex();
}

QModelIndex Models::MessageFeed::modelIndexByTime(const QString& id, const QDateTime& time) const
{
    StorageByTime::const_iterator tItr = indexByTime.upper_bound(time);
    StorageByTime::const_iterator tBeg = indexByTime.begin();
    bool found = false;
    while (tItr != tBeg) {
        if (id == (*tItr)->getId()) {
            found = true;
            break;
        }
        --tItr;
    }
    
    if (found) {
        int position = indexByTime.rank(tItr);
        return createIndex(position, 0, *tItr);
    }
    
    return QModelIndex();
}