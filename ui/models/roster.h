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

#ifndef MODELS_ROSTER_H
#define MODELS_ROSTER_H

#include <qabstractitemmodel.h>
#include <deque>
#include <map>
#include <QVector>

#include "shared/message.h"
#include "shared/global.h"
#include "shared/messageinfo.h"
#include "accounts.h"
#include "item.h"
#include "account.h"
#include "contact.h"
#include "group.h"
#include "room.h"
#include "reference.h"

namespace Models
{

class Roster : public QAbstractItemModel
{
    Q_OBJECT
public:
    class ElId;
    Roster(QObject* parent = 0);
    ~Roster();

    void addAccount(const QMap<QString, QVariant> &data);
    void updateAccount(const QString& account, const QString& field, const QVariant& value);
    void removeAccount(const QString& account);
    void addGroup(const QString& account, const QString& name);
    void removeGroup(const QString& account, const QString& name);
    void addContact(const QString& account, const QString& jid, const QString& group, const QMap<QString, QVariant>& data);
    void removeContact(const QString& account, const QString& jid, const QString& group);
    void removeContact(const QString& account, const QString& jid);
    void changeContact(const QString& account, const QString& jid, const QMap<QString, QVariant>& data);
    void addPresence(const QString& account, const QString& jid, const QString& name, const QMap<QString, QVariant>& data);
    void removePresence(const QString& account, const QString& jid, const QString& name);
    void addMessage(const QString& account, const Shared::Message& data);
    void changeMessage(const QString& account, const QString& jid, const QString& id, const QMap<QString, QVariant>& data);
    void addRoom(const QString& account, const QString jid, const QMap<QString, QVariant>& data);
    void changeRoom(const QString& account, const QString jid, const QMap<QString, QVariant>& data);
    void removeRoom(const QString& account, const QString jid);
    void addRoomParticipant(const QString& account, const QString& jid, const QString& name, const QMap<QString, QVariant>& data);
    void changeRoomParticipant(const QString& account, const QString& jid, const QString& name, const QMap<QString, QVariant>& data);
    void removeRoomParticipant(const QString& account, const QString& jid, const QString& name);
    QString getContactName(const QString& account, const QString& jid);
    
    QVariant data ( const QModelIndex& index, int role ) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    int columnCount ( const QModelIndex& parent ) const override;
    int rowCount ( const QModelIndex& parent ) const override;
    QModelIndex parent ( const QModelIndex& child ) const override;
    QModelIndex index ( int row, int column, const QModelIndex& parent ) const override;
    
    std::deque<QString> groupList(const QString& account) const;
    bool groupHasContact(const QString& account, const QString& group, const QString& contactJID) const;
    QString getContactIconPath(const QString& account, const QString& jid, const QString& resource);
    Account* getAccount(const QString& name);
    QModelIndex getAccountIndex(const QString& name);
    QModelIndex getGroupIndex(const QString& account, const QString& name);
    void responseArchive(const QString& account, const QString& jid, const std::list<Shared::Message>& list, bool last);
    
    void fileProgress(const std::list<Shared::MessageInfo>& msgs, qreal value, bool up);
    void fileError(const std::list<Shared::MessageInfo>& msgs, const QString& err, bool up);
    void fileComplete(const std::list<Shared::MessageInfo>& msgs, bool up);
    
    Accounts* accountsModel;
    
signals:
    void requestArchive(const QString& account, const QString& jid, const QString& before);
    void fileDownloadRequest(const QString& url);
    
private:
    Element* getElement(const ElId& id);
    
private slots:
    void onAccountDataChanged(const QModelIndex& tl, const QModelIndex& br, const QVector<int>& roles);
    void onChildChanged(Models::Item* item, int row, int col);
    void onChildIsAboutToBeInserted(Item* parent, int first, int last);
    void onChildInserted();
    void onChildIsAboutToBeRemoved(Item* parent, int first, int last);
    void onChildRemoved();
    void onChildIsAboutToBeMoved(Item* source, int first, int last, Item* destination, int newIndex);
    void onChildMoved();
    void onElementRequestArchive(const QString& before);
    
private:
    Item* root;
    std::map<QString, Account*> accounts;
    std::map<ElId, Group*> groups;
    std::map<ElId, Contact*> contacts;
    std::map<ElId, Room*> rooms;
    
public:
    class ElId {
    public:
        ElId (const QString& p_account = "", const QString& p_name = "");
        
        const QString account;
        const QString name;
        
        bool operator < (const ElId& other) const;
        bool operator == (const ElId& other) const;
        bool operator != (const ElId& other) const;
    };
};

}

#endif // MODELS_ROSTER_H
