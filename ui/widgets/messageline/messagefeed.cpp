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

#include <ui/models/element.h>
#include <ui/models/room.h>

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
    {Error, "error"},
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
    downloads(),
    failedDownloads(),
    failedUploads(),
    unreadMessages(new std::set<QString>()),
    observersAmount(0)
{
}

Models::MessageFeed::~MessageFeed()
{
    delete unreadMessages;
    
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
    
    emit newMessage(msg);
    
    if (observersAmount == 0 && !msg.getForwarded()) {      //not to notify when the message is delivered by the carbon copy
        unreadMessages->insert(msg.getId());                //cuz it could be my own one or the one I read on another device
        emit unreadMessagesCountChanged();
        emit unnoticedMessage(msg);
    }
}

void Models::MessageFeed::changeMessage(const QString& id, const QMap<QString, QVariant>& data)
{
    StorageById::iterator itr = indexById.find(id);
    if (itr == indexById.end()) {
        qDebug() << "received a command to change a message, but the message couldn't be found, skipping";
        return;
    }
    
    Shared::Message* msg = *itr;
    std::set<MessageRoles> changeRoles = detectChanges(*msg, data);
    QModelIndex index = modelIndexByTime(id, msg->getTime());
    Shared::Message::Change functor(data);
    bool success = indexById.modify(itr, functor);
    if (!success) {
        qDebug() << "received a command to change a message, but something went wrong modifying message in the feed, throwing error";
        throw 872;
    }
    
    if (functor.hasIdBeenModified()) {
        changeRoles.insert(MessageRoles::Id);
        std::set<QString>::const_iterator umi = unreadMessages->find(id);
        if (umi != unreadMessages->end()) {
            unreadMessages->erase(umi);
            unreadMessages->insert(msg->getId());
        }
    }
    
    if (changeRoles.size() > 0) {
        //change message is a final event in download/upload event train
        //only after changeMessage we can consider the download is done
        Progress::const_iterator dItr = downloads.find(id);
        bool attachOrError = changeRoles.count(MessageRoles::Attach) > 0 || changeRoles.count(MessageRoles::Error);
        if (dItr != downloads.end()) {
            if (attachOrError) {
                downloads.erase(dItr);
            } else if (changeRoles.count(MessageRoles::Id) > 0) {
                qreal progress = dItr->second;
                downloads.erase(dItr);
                downloads.insert(std::make_pair(msg->getId(), progress));
            }
        } else {
            dItr = uploads.find(id);
            if (dItr != uploads.end()) {
                if (attachOrError) {
                    uploads.erase(dItr);
                } else if (changeRoles.count(MessageRoles::Id) > 0) {
                    qreal progress = dItr->second;
                    uploads.erase(dItr);
                    uploads.insert(std::make_pair(msg->getId(), progress));
                }
            }
        }
        
        Err::const_iterator eitr = failedDownloads.find(id);
        if (eitr != failedDownloads.end()) {
            failedDownloads.erase(eitr);
            changeRoles.insert(MessageRoles::Attach);
        } else {
            eitr = failedUploads.find(id);
            if (eitr != failedUploads.end()) {
                failedUploads.erase(eitr);
                changeRoles.insert(MessageRoles::Attach);
            }
        }
        
        QVector<int> cr;
        for (MessageRoles role : changeRoles) {
            cr.push_back(role);
        }
        
        emit dataChanged(index, index, cr);
    }
}

std::set<Models::MessageFeed::MessageRoles> Models::MessageFeed::detectChanges(const Shared::Message& msg, const QMap<QString, QVariant>& data) const
{
    std::set<MessageRoles> roles;
    Shared::Message::State state = msg.getState();
    QMap<QString, QVariant>::const_iterator itr = data.find("state");
    if (itr != data.end() && static_cast<Shared::Message::State>(itr.value().toUInt()) != state) {
        roles.insert(MessageRoles::DeliveryState);
    }
    
    itr = data.find("outOfBandUrl");
    bool att = false;
    if (itr != data.end() && itr.value().toString() != msg.getOutOfBandUrl()) {
        roles.insert(MessageRoles::Attach);
        att = true;
    }
    
    if (!att) {
        itr = data.find("attachPath");
        if (itr != data.end() && itr.value().toString() != msg.getAttachPath()) {
            roles.insert(MessageRoles::Attach);
        }
    }
    
    if (state == Shared::Message::State::error) {
        itr = data.find("errorText");
        if (itr != data.end() && itr.value().toString() != msg.getErrorText()) {
            roles.insert(MessageRoles::Error);
        }
    }
    
    itr = data.find("body");
    if (itr != data.end() && itr.value().toString() != msg.getBody()) {
        QMap<QString, QVariant>::const_iterator dItr = data.find("stamp");
        QDateTime correctionDate;
        if (dItr != data.end()) {
            correctionDate = dItr.value().toDateTime();
        } else {
            correctionDate = QDateTime::currentDateTimeUtc();      //in case there is no information about time of this correction it's applied
        }
        if (!msg.getEdited() || msg.getLastModified() < correctionDate) {
            roles.insert(MessageRoles::Text);
            roles.insert(MessageRoles::Correction);
        }
    } else {
        QMap<QString, QVariant>::const_iterator dItr = data.find("stamp");
        if (dItr != data.end()) {
            QDateTime ntime = dItr.value().toDateTime();
            if (msg.getTime() != ntime) {
                roles.insert(MessageRoles::Date);
            }
        }
    }
    
    return roles;
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
                answer.setValue(fillCorrection(*msg));;
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
                break;
            case Error: 
                answer.setValue(msg->getErrorText());
                break;
            case Bulk: {
                FeedItem item;
                item.id = msg->getId();
                
                std::set<QString>::const_iterator umi = unreadMessages->find(item.id);
                if (umi != unreadMessages->end()) {
                    unreadMessages->erase(umi);
                    emit unreadMessagesCountChanged();
                }
                
                item.sentByMe = sentByMe(*msg);
                item.date = msg->getTime();
                item.state = msg->getState();
                item.error = msg->getErrorText();
                item.correction = fillCorrection(*msg);
                
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
    return unreadMessages->size();
}

bool Models::MessageFeed::canFetchMore(const QModelIndex& parent) const
{
    return syncState == incomplete;
}

void Models::MessageFeed::fetchMore(const QModelIndex& parent)
{
    if (syncState == incomplete) {
        syncState = syncing;
        emit syncStateChange(syncState);
        
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
        emit syncStateChange(syncState);
    }
}

QModelIndex Models::MessageFeed::index(int row, int column, const QModelIndex& parent) const
{
    if (!hasIndex(row, column, parent)) {
        return QModelIndex();
    }
    
    StorageByTime::iterator itr = indexByTime.nth(row);
    if (itr != indexByTime.end()) {
        Shared::Message* msg = *itr;
        
        return createIndex(row, column, msg);
    } else {
        return QModelIndex();
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
    QString id = msg.getId();
    
    att.localPath = msg.getAttachPath();
    att.remotePath = msg.getOutOfBandUrl();
    
    if (att.remotePath.size() == 0) {
        if (att.localPath.size() == 0) {
            att.state = none;
        } else {
            Err::const_iterator eitr = failedUploads.find(id);
            if (eitr != failedUploads.end()) {
                att.state = errorUpload;
                att.error = eitr->second;
            } else {
                Progress::const_iterator itr = uploads.find(id);
                if (itr == uploads.end()) {
                    att.state = local;
                } else {
                    att.state = uploading;
                    att.progress = itr->second;
                }
            }
        }
    } else {
        if (att.localPath.size() == 0) {
            Err::const_iterator eitr = failedDownloads.find(id);
            if (eitr != failedDownloads.end()) {
                att.state = errorDownload;
                att.error = eitr->second;
            } else {
                Progress::const_iterator itr = downloads.find(id);
                if (itr == downloads.end()) {
                    att.state = remote;
                } else {
                    att.state = downloading;
                    att.progress = itr->second;
                }
            }
        } else {
            att.state = ready;
        }
    }
    
    return att;
}

Models::Edition Models::MessageFeed::fillCorrection(const Shared::Message& msg) const
{
    ::Models::Edition ed({msg.getEdited(), msg.getOriginalBody(), msg.getLastModified()});
    
    return ed;
}


void Models::MessageFeed::downloadAttachment(const QString& messageId)
{
    bool notify = false;
    Err::const_iterator eitr = failedDownloads.find(messageId);
    if (eitr != failedDownloads.end()) {
        failedDownloads.erase(eitr);
        notify = true;
    }
    
    QModelIndex ind = modelIndexById(messageId);
    if (ind.isValid()) {
        std::pair<Progress::iterator, bool> progressPair = downloads.insert(std::make_pair(messageId, 0));
        if (progressPair.second) {     //Only to take action if we weren't already downloading it
            Shared::Message* msg = static_cast<Shared::Message*>(ind.internalPointer());
            notify = true;
            emit fileDownloadRequest(msg->getOutOfBandUrl());
        } else {
            qDebug() << "Attachment download for message with id" << messageId << "is already in progress, skipping";
        }
    } else {
        qDebug() << "An attempt to download an attachment for the message that doesn't exist. ID:" << messageId;
    }
    
    if (notify) {
        emit dataChanged(ind, ind, {MessageRoles::Attach});
    }
}

bool Models::MessageFeed::registerUpload(const QString& messageId)
{
    bool success = uploads.insert(std::make_pair(messageId, 0)).second; 
    
    QVector<int> roles({});
    Err::const_iterator eitr = failedUploads.find(messageId);
    if (eitr != failedUploads.end()) {
        failedUploads.erase(eitr);
        roles.push_back(MessageRoles::Attach);
    } else if (success) {
        roles.push_back(MessageRoles::Attach);
    }
    
    QModelIndex ind = modelIndexById(messageId);
    emit dataChanged(ind, ind, roles);
    
    return success;
}

void Models::MessageFeed::fileProgress(const QString& messageId, qreal value, bool up)
{
    Progress* pr = 0;
    Err* err = 0;
    if (up) {
        pr = &uploads;
        err = &failedUploads;
    } else {
        pr = &downloads;
        err = &failedDownloads;
    }
    
    QVector<int> roles({});
    Err::const_iterator eitr = err->find(messageId);
    if (eitr != err->end() && value != 1) {         //like I want to clear this state when the download is started anew
        err->erase(eitr);
        roles.push_back(MessageRoles::Attach);
    }
    
    Progress::iterator itr = pr->find(messageId);
    if (itr != pr->end()) {
        itr->second = value;
        QModelIndex ind = modelIndexById(messageId);
        emit dataChanged(ind, ind, roles);
    }
}

void Models::MessageFeed::fileComplete(const QString& messageId, bool up)
{
    fileProgress(messageId, 1, up);
}

void Models::MessageFeed::fileError(const QString& messageId, const QString& error, bool up)
{
    Err* failed;
    Progress* loads;
    if (up) {
        failed = &failedUploads;
        loads = &uploads;
    } else {
        failed = &failedDownloads;
        loads = &downloads;
    }
    
    Progress::iterator pitr = loads->find(messageId);
    if (pitr != loads->end()) {
        loads->erase(pitr);
    }
    
    std::pair<Err::iterator, bool> pair = failed->insert(std::make_pair(messageId, error));
    if (!pair.second) {
        pair.first->second = error;
    }
    QModelIndex ind = modelIndexById(messageId);
    if (ind.isValid()) {
        emit dataChanged(ind, ind, {MessageRoles::Attach});
    }
}

void Models::MessageFeed::incrementObservers()
{
    ++observersAmount;
}

void Models::MessageFeed::decrementObservers()
{
    --observersAmount;
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
    if (indexByTime.size() > 0) {
        StorageByTime::const_iterator tItr = indexByTime.lower_bound(time);
        StorageByTime::const_iterator tEnd = indexByTime.upper_bound(time);
        bool found = false;
        while (tItr != tEnd) {
            if (id == (*tItr)->getId()) {
                found = true;
                break;
            }
            ++tItr;
        }
        
        if (found) {
            int position = indexByTime.rank(tItr);
            return createIndex(position, 0, *tItr);
        }
    }
    
    return QModelIndex();
}

void Models::MessageFeed::reportLocalPathInvalid(const QString& messageId)
{
    StorageById::iterator itr = indexById.find(messageId);
    if (itr == indexById.end()) {
        qDebug() << "received a command to change a message, but the message couldn't be found, skipping";
        return;
    }
    
    Shared::Message* msg = *itr;
    
    emit localPathInvalid(msg->getAttachPath());
    
    //gonna change the message in current model right away, to prevent spam on each attempt to draw element
    QModelIndex index = modelIndexByTime(messageId, msg->getTime());
    msg->setAttachPath("");
    
    emit dataChanged(index, index, {MessageRoles::Attach});
}

Models::MessageFeed::SyncState Models::MessageFeed::getSyncState() const
{
    return syncState;
}

void Models::MessageFeed::requestLatestMessages()
{
    if (syncState != syncing) {
        syncState = syncing;
        emit syncStateChange(syncState);
        
        emit requestArchive("");
    }
}
