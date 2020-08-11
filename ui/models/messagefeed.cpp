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

#include <QDebug>

MessageFeed::MessageFeed(QObject* parent):
    QAbstractListModel(parent),
    storage(),
    indexById(storage.get<id>()),
    indexByTime(storage.get<time>()),
    syncState(incomplete)
{
}

MessageFeed::~MessageFeed()
{
    for (Shared::Message* message : storage) {
        delete message;
    }
}

void MessageFeed::addMessage(const Shared::Message& msg)
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

void MessageFeed::changeMessage(const QString& id, const Shared::Message& msg)
{
}

void MessageFeed::removeMessage(const QString& id)
{
}

QVariant MessageFeed::data(const QModelIndex& index, int role) const
{
    int i = index.row();
    if (syncState == syncing) {
        --i;
    }
    QVariant answer;
    switch (role) {
        case Qt::DisplayRole: {
            if (i == -1) {
                return "Loading...";
            }
            
            StorageByTime::const_iterator itr = indexByTime.nth(i);
            if (itr != indexByTime.end()) {
                const Shared::Message* msg = *itr;
                answer = msg->getFrom() + ": " + msg->getBody();
            }
        }
            break;
        default:
            break;
    }
    
    return answer;
}

int MessageFeed::rowCount(const QModelIndex& parent) const
{
    int count = storage.size();
    if (syncState == syncing) {
        count++;
    }
    return count;
}

unsigned int MessageFeed::unreadMessagesCount() const
{
    return storage.size(); //let's say they are all new for now =)
}

bool MessageFeed::canFetchMore(const QModelIndex& parent) const
{
    return syncState == incomplete;
}

void MessageFeed::fetchMore(const QModelIndex& parent)
{
    if (syncState == incomplete) {
        beginInsertRows(QModelIndex(), 0, 0);
        syncState = syncing;
        endInsertRows();
        
        if (storage.size() == 0) {
            emit requestArchive("");
        } else {
            emit requestArchive((*indexByTime.nth(0))->getId());
        }
    }
}

void MessageFeed::responseArchive(const std::list<Shared::Message> list)
{
    if (syncState == syncing) {
        beginRemoveRows(QModelIndex(), 0, 0);
        syncState = incomplete;
        endRemoveRows();
    }
    
    beginInsertRows(QModelIndex(), 0, list.size() - 1);
    for (const Shared::Message& msg : list) {
        Shared::Message* copy = new Shared::Message(msg);
        storage.insert(copy);
    }
    endInsertRows();
}

