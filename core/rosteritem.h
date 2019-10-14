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

#ifndef CORE_ROSTERITEM_H
#define CORE_ROSTERITEM_H

#include <QObject>
#include <QString>
#include <QStandardPaths>

#include <list>

#include "../global.h"
#include "archive.h"

namespace Core {

/**
 * @todo write docs
 */
class RosterItem : public QObject
{
    Q_OBJECT
public:
    enum ArchiveState {
        empty,              //have no messages stored for this contact
        chunk,              //have some chunk of history, don't have the beginning nor have the end
        beginning,          //have the history from the very beginning, don't have the end
        end,                //have the history to the end, but don't have the beginning
        complete            //have full history locally stored
    };
    
    RosterItem(const QString& pJid, const QString& account, QObject* parent = 0);
    ~RosterItem();
    
    ArchiveState getArchiveState() const;
    QString getName() const;
    void setName(const QString& n);
    QString getServer() const;
    bool isMuc() const;
    
    void addMessageToArchive(const Shared::Message& msg);
    void appendMessageToArchive(const Shared::Message& msg);
    void flushMessagesToArchive(bool finished, const QString& firstId, const QString& lastId);
    void requestHistory(int count, const QString& before);
    void requestFromEmpty(int count, const QString& before);
    bool hasAvatar() const;
    QString avatarHash() const;
    QString avatarPath() const;
    
signals:
    void nameChanged(const QString& name);
    void subscriptionStateChanged(Shared::SubscriptionState state);
    void historyResponse(const std::list<Shared::Message>& messages);
    void needHistory(const QString& before, const QString& after, const QDateTime& afterTime = QDateTime());
    
public:
    const QString jid;
    const QString account;
    
protected:
    QString name;
    ArchiveState archiveState;
    Archive* archive;

    bool syncronizing;
    int requestedCount;
    QString requestedBefore;
    std::list<Shared::Message> hisoryCache;
    std::list<Shared::Message> appendCache;
    std::list<Shared::Message> responseCache;
    std::list<std::pair<int, QString>> requestCache;
    bool muc;

private:
    void nextRequest();
    void performRequest(int count, const QString& before);
};

}

#endif // CORE_ROSTERITEM_H
