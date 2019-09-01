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

#include "roster.h"
#include <QDebug>
#include <QIcon>
#include <QFont>

using namespace Models;

Models::Roster::Roster(QObject* parent):
    QAbstractItemModel(parent),
    accountsModel(new Accounts()),
    root(new Item(Item::root, {{"name", "root"}})),
    accounts(),
    groups(),
    contacts()
{
    connect(accountsModel, 
            SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&, const QVector<int>&)), 
            this, 
            SLOT(onAccountDataChanged(const QModelIndex&, const QModelIndex&, const QVector<int>&)));
    connect(root, SIGNAL(childChanged(Models::Item*, int, int)), this, SLOT(onChildChanged(Models::Item*, int, int)));
    connect(root, SIGNAL(childIsAboutToBeInserted(Item*, int, int)), this, SLOT(onChildIsAboutToBeInserted(Item*, int, int)));
    connect(root, SIGNAL(childInserted()), this, SLOT(onChildInserted()));
    connect(root, SIGNAL(childIsAboutToBeRemoved(Item*, int, int)), this, SLOT(onChildIsAboutToBeRemoved(Item*, int, int)));
    connect(root, SIGNAL(childRemoved()), this, SLOT(onChildRemoved()));
    connect(root, SIGNAL(childIsAboutToBeMoved(Item*, int, int, Item*, int)), this, SLOT(onChildIsAboutToBeMoved(Item*, int, int, Item*, int)));
    connect(root, SIGNAL(childMoved()), this, SLOT(onChildMoved()));
}

Models::Roster::~Roster()
{
    delete accountsModel;
    delete root;
}

void Models::Roster::addAccount(const QMap<QString, QVariant>& data)
{
    Account* acc = new Account(data);
    root->appendChild(acc);
    accounts.insert(std::make_pair(acc->getName(), acc));
    accountsModel->addAccount(acc);
}

QVariant Models::Roster::data (const QModelIndex& index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }
    
    QVariant result;
    
    Item *item = static_cast<Item*>(index.internalPointer());
    switch (role) {
        case Qt::DisplayRole:
        {
            switch (item->type) {
                case Item::group: {
                    Group* gr = static_cast<Group*>(item);
                    QString str("");
                    
                    str += gr->getName();
                    unsigned int amount = gr->getUnreadMessages();
                    if (amount > 0) {
                        str += QString(" (") +  "New messages" + ")";
                    }
                    
                    result = str;
                }
                    break;
                default:
                    result = item->data(index.column());
                    break;
            }
        }
            break;
        case Qt::DecorationRole:
            switch (item->type) {
                case Item::account: {
                    Account* acc = static_cast<Account*>(item);
                    result = acc->getStatusIcon(false);
                }
                    break;
                case Item::contact: {
                    Contact* contact = static_cast<Contact*>(item);
                    result = contact->getStatusIcon(false);
                }
                    break;
                case Item::presence: {
                    Presence* presence = static_cast<Presence*>(item);
                    result = presence->getStatusIcon(false);
                }
                    break;
                case Item::room: {
                    Room* room = static_cast<Room*>(item);
                    result = room->getStatusIcon(false);
                }
                    break;
                default:
                    break;
            }
            break;
        case Qt::FontRole:
            switch (item->type) {
                case Item::account:{
                    QFont font;
                    font.setBold(true);
                    result = font;
                }
                    break;
                case Item::group:{
                    QFont font;
                    font.setItalic(true);
                    result = font;
                }
                    break;
                default:
                    break;
            }
            break;
        case Qt::ToolTipRole:
            switch (item->type) {
                case Item::account: {
                    Account* acc = static_cast<Account*>(item);
                    result = QString(Shared::availabilityNames[acc->getAvailability()]);
                }
                    break;
                case Item::contact: {
                    Contact* contact = static_cast<Contact*>(item);
                    QString str("");
                    int mc = contact->getMessagesCount();
                    if (mc > 0) {
                        str += QString("New messages: ") + std::to_string(mc).c_str() + "\n";
                    }
                    Shared::SubscriptionState ss = contact->getState();
                    if (ss == Shared::both) {
                        Shared::Availability av = contact->getAvailability();
                        str += "Availability: " + Shared::availabilityNames[av];
                        if (av != Shared::offline) {
                            QString s = contact->getStatus();
                            if (s.size() > 0) {
                                str += "\nStatus: " + s;
                            }
                        }
                        str += "\nSubscription: " + Shared::subscriptionStateNames[ss];
                    } else {
                        str += "Subscription: " + Shared::subscriptionStateNames[ss];
                    }
                    
                    result = str;
                }
                    break;
                case Item::presence: {
                    Presence* contact = static_cast<Presence*>(item);
                    QString str("");
                    int mc = contact->getMessagesCount();
                    if (mc > 0) {
                        str += QString("New messages: ") + std::to_string(mc).c_str() + "\n";
                    }
                    Shared::Availability av = contact->getAvailability();
                    str += "Availability: " + Shared::availabilityNames[av];
                    QString s = contact->getStatus();
                    if (s.size() > 0) {
                        str += "\nStatus: " + s;
                    }
                    
                    result = str;
                }
                    break;
                case Item::group: {
                    Group* gr = static_cast<Group*>(item);
                    unsigned int count = gr->getUnreadMessages();
                    QString str("");
                    if (count > 0) {
                        str += QString("New messages: ") + std::to_string(count).c_str() + "\n";
                    }
                    str += QString("Online contacts: ") + std::to_string(gr->getOnlineContacts()).c_str() + "\n";
                    str += QString("Total contacts: ") + std::to_string(gr->childCount()).c_str();
                    result = str;
                }
                    break;
                case Item::room: {
                    Room* rm = static_cast<Room*>(item);
                    unsigned int count = rm->getUnreadMessagesCount();
                    QString str("");
                    if (count > 0) {
                        str += QString("New messages: ") + std::to_string(count).c_str() + "\n";
                    }
                    str += QString("Subscription: ") + rm->getStatusText();
                    result = str;
                }
                    break;
                default:
                    result = "";
                    break;
            }
            break;
        default:
            break;
    }
    
    return result;
}

int Models::Roster::columnCount (const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return static_cast<Item*>(parent.internalPointer())->columnCount();
    } else {
        return root->columnCount();
    }
}

void Models::Roster::updateAccount(const QString& account, const QString& field, const QVariant& value)
{
    std::map<QString, Account*>::iterator itr = accounts.find(account);
    if (itr != accounts.end()) {
        Account* acc = itr->second;
        acc->update(field, value);
    }
}


Qt::ItemFlags Models::Roster::flags(const QModelIndex& index) const
{
    if (!index.isValid()) {
        return 0;
    }
    
    return QAbstractItemModel::flags(index);
}


int Models::Roster::rowCount (const QModelIndex& parent) const
{
    Item *parentItem;
    if (parent.column() > 0) {
        return 0;
    }
    
    if (!parent.isValid()) {
        parentItem = root;
    } else {
        parentItem = static_cast<Item*>(parent.internalPointer());
    }
    
    return parentItem->childCount();
}

QVariant Models::Roster::headerData(int section, Qt::Orientation orientation, int role) const
{
    return QVariant();
}

QModelIndex Models::Roster::parent (const QModelIndex& child) const
{
    if (!child.isValid()) {
        return QModelIndex();
    }
    
    Item *childItem = static_cast<Item*>(child.internalPointer());
    if (childItem == root) {
        return QModelIndex();
    }
    
    Item *parentItem = childItem->parentItem();
    
    if (parentItem == root) {
        return createIndex(0, 0, parentItem);
    }
    
    return createIndex(parentItem->row(), 0, parentItem);
}

QModelIndex Models::Roster::index (int row, int column, const QModelIndex& parent) const
{
    if (!hasIndex(row, column, parent)) {
        return QModelIndex();
    }
    
    Item *parentItem;
    
    if (!parent.isValid()) {
        parentItem = root;
    } else {
        parentItem = static_cast<Item*>(parent.internalPointer());
    }
    
    Item *childItem = parentItem->child(row);
    if (childItem) {
        return createIndex(row, column, childItem);
    } else {
        return QModelIndex();
    }
}

Models::Roster::ElId::ElId(const QString& p_account, const QString& p_name):
    account(p_account),
    name(p_name)
{}

bool Models::Roster::ElId::operator <(const Models::Roster::ElId& other) const
{
    if (account == other.account) {
        return name < other.name;
    } else {
        return account < other.account;
    }
}

void Models::Roster::onAccountDataChanged(const QModelIndex& tl, const QModelIndex& br, const QVector<int>& roles)
{
    if (tl.column() == 0) {
        emit dataChanged(tl, br, roles);
    } else if (tl.column() == 2) {
        int row = tl.row();
        Account* acc = accountsModel->getAccount(row);
        emit dataChanged(createIndex(row, 0, acc), createIndex(br.row(), 0, acc), roles);
    }
}

void Models::Roster::addGroup(const QString& account, const QString& name)
{
    ElId id(account, name);
    std::map<ElId, Group*>::const_iterator gItr = groups.find(id);
    if (gItr != groups.end()) {
        qDebug() << "An attempt to add group " << name << " to account " << account <<" which already exists there, skipping";
        return;
    }
    std::map<QString, Account*>::iterator itr = accounts.find(account);
    if (itr != accounts.end()) {
        Account* acc = itr->second;
        Group* group = new Group({{"name", name}});
        groups.insert(std::make_pair(id, group));
        acc->appendChild(group);
    } else {
        qDebug() << "An attempt to add group " << name << " to non existing account " << account << ", skipping";
    }
}

void Models::Roster::addContact(const QString& account, const QString& jid, const QString& group, const QMap<QString, QVariant>& data)
{
    Item* parent;
    Account* acc;
    Contact* contact;
    ElId id(account, jid);
    
    {
        std::map<QString, Account*>::iterator itr = accounts.find(account);
        if (itr == accounts.end()) {
            qDebug() << "An attempt to add a contact " << jid << " to non existing account " << account << ", skipping";
            return;
        }
        acc = itr->second;
    }
    
    if (group == "") {
        std::multimap<ElId, Contact*>::iterator itr = contacts.lower_bound(id);
        std::multimap<ElId, Contact*>::iterator eItr = contacts.upper_bound(id);
        while (itr != eItr) {
            if (itr->second->parentItem() == acc) {
                qDebug() << "An attempt to add a contact " << jid << " ungrouped to non the account " << account << " for the second time, skipping";
                return;
            }
            itr++;
        }
        parent = acc;
    } else {
        std::map<ElId, Group*>::iterator itr = groups.find({account, group});
        if (itr == groups.end()) {
            qDebug() << "An attempt to add a contact " << jid << " to non existing group " << group << ", skipping";
            return;
        }
        
        parent = itr->second;
        
        for (int i = 0; i < parent->childCount(); ++i) {
            Item* item = parent->child(i);
            if (item->type == Item::contact) {
                Contact* ca = static_cast<Contact*>(item);
                if (ca->getJid() == jid) {
                    qDebug() << "An attempt to add a contact " << jid << " to the group " << group << " for the second time, skipping";
                    return;
                }
            }
        }
        
        for (int i = 0; i < acc->childCount(); ++i) {
            Item* item = acc->child(i);
            if (item->type == Item::contact) {
                Contact* ca = static_cast<Contact*>(item);
                if (ca->getJid() == jid) {
                    qDebug() << "An attempt to add a already existing contact " << jid << " to the group " << group << ", contact will be moved from ungrouped contacts of " << account;
                    
                    parent->appendChild(ca);
                    return;
                }
            }
        }
        
    }
    contact = new Contact(jid, data);
    contacts.insert(std::make_pair(id, contact));
    parent->appendChild(contact);
}

void Models::Roster::removeGroup(const QString& account, const QString& name)
{
    ElId id(account, name);
    std::map<ElId, Group*>::const_iterator gItr = groups.find(id);
    if (gItr == groups.end()) {
        qDebug() << "An attempt to remove group " << name << " from account " << account <<" which doesn't exist there, skipping";
        return;
    }
    Group* item = gItr->second;
    Item* parent = item->parentItem();
    int row = item->row();
    
    parent->removeChild(row);
    
    std::deque<Contact*> toInsert;
    for (int i = 0; item->childCount() > 0; ++i) {
        Contact* cont = static_cast<Contact*>(item->child(0));
        item->removeChild(0);
        
        std::multimap<ElId, Contact*>::iterator cBeg = contacts.lower_bound({account, cont->getJid()});
        std::multimap<ElId, Contact*>::iterator cEnd = contacts.upper_bound({account, cont->getJid()});
        
        int count = std::distance(cBeg, cEnd);
        if (count > 1) {
            for (; cBeg != cEnd; ++count, ++cBeg) {
                if (cBeg->second == cont) {
                    delete cont;
                    contacts.erase(cBeg);
                    break;
                }
            }
        } else {
            toInsert.push_back(cont);
        }
    }
    
    if (toInsert.size() > 0) {
        Account* acc = accounts.find("account")->second;
        for (int i = 0; i < toInsert.size(); ++i) {
            Contact* cont = toInsert[i];
            acc->appendChild(cont);             //TODO optimisation
        }
    }
    
    item->deleteLater();
    groups.erase(gItr);
}

void Models::Roster::changeContact(const QString& account, const QString& jid, const QMap<QString, QVariant>& data)
{
    ElId id(account, jid);
    std::multimap<ElId, Contact*>::iterator cBeg = contacts.lower_bound(id);
    std::multimap<ElId, Contact*>::iterator cEnd = contacts.upper_bound(id);
    
    for (; cBeg != cEnd; ++cBeg) {
        for (QMap<QString, QVariant>::const_iterator itr = data.begin(), end = data.end(); itr != end; ++itr) {
            cBeg->second->update(itr.key(), itr.value());;
        }
    }
    
    std::map<ElId, Room*>::iterator rItr = rooms.find(id);
    if (rItr != rooms.end()) {
        for (QMap<QString, QVariant>::const_iterator itr = data.begin(), end = data.end(); itr != end; ++itr) {
            rItr->second->update(itr.key(), itr.value());;
        }
    }
}

void Models::Roster::removeContact(const QString& account, const QString& jid)
{
    ElId id(account, jid);
    std::multimap<ElId, Contact*>::iterator cBeg = contacts.lower_bound(id);
    std::multimap<ElId, Contact*>::iterator cEnd = contacts.upper_bound(id);
    std::multimap<ElId, Contact*>::iterator cpBeg = cBeg;
    
    QSet<QString> toRemove;
    for (; cBeg != cEnd; ++cBeg) {
        Contact* contact = cBeg->second;
        Item* parent = contact->parentItem();
        if (parent->type == Item::group && parent->childCount() == 1) {
            toRemove.insert(parent->getName());
        }
        
        parent->removeChild(contact->row());
        contact->deleteLater();
    }
    contacts.erase(cpBeg, cEnd);
    
    for (QSet<QString>::const_iterator itr = toRemove.begin(), end = toRemove.end(); itr != end; ++itr) {
        removeGroup(account, *itr);
    }
}

void Models::Roster::removeContact(const QString& account, const QString& jid, const QString& group)
{
    ElId contactId(account, jid);
    ElId groupId(account, group);
    
    std::map<ElId, Group*>::iterator gItr = groups.find(groupId);
    if (gItr == groups.end()) {
        qDebug() << "An attempt to remove contact " << jid << " from non existing group " << group << " of account " << account <<", skipping";
        return;
    }
    Group* gr = gItr->second;
    Contact* cont = 0;
    
    std::multimap<ElId, Contact*>::iterator cBeg = contacts.lower_bound(contactId);
    std::multimap<ElId, Contact*>::iterator cEnd = contacts.upper_bound(contactId);
    for (;cBeg != cEnd; ++cBeg) {
        if (cBeg->second->parentItem() == gr) {
            cont = cBeg->second;
            contacts.erase(cBeg);
            break;
        }
    }
    
    if (cont == 0) {
        qDebug() << "An attempt to remove contact " << jid << " of account " << account << " from group " << group  <<", but there is no such contact in that group, skipping";
        return;
    }
    
    gr->removeChild(cont->row());
    cont->deleteLater();
    
    if (gr->childCount() == 0) {
        removeGroup(account, group);
    }
}

void Models::Roster::onChildChanged(Models::Item* item, int row, int col)
{
    QModelIndex index = createIndex(row, 0, item);
    emit dataChanged(index, index);
}

void Models::Roster::onChildIsAboutToBeInserted(Models::Item* parent, int first, int last)
{
    int row = 0;
    if (parent != root) {
        row = parent->row();
        beginInsertRows(createIndex(row, 0, parent), first, last);
    } else {
        beginInsertRows(QModelIndex(), first, last);
    }
}

void Models::Roster::onChildIsAboutToBeMoved(Models::Item* source, int first, int last, Models::Item* destination, int newIndex)
{
    int oldRow = 0;
    if (source != root) {
        oldRow = source->row();
    }
    int newRow = 0;
    if (destination != root) {
        newRow = destination->row();
    }
    beginMoveRows(createIndex(oldRow, 0, source), first, last, createIndex(newRow, 0, destination), newIndex);
}

void Models::Roster::onChildIsAboutToBeRemoved(Models::Item* parent, int first, int last)
{
    int row = 0;
    if (parent != root) {
        row = parent->row();
    }
    beginRemoveRows(createIndex(row, 0, parent), first, last);
}

void Models::Roster::onChildInserted()
{
    endInsertRows();
}

void Models::Roster::onChildMoved()
{
    endMoveRows();
}

void Models::Roster::onChildRemoved()
{
    endRemoveRows();
}

void Models::Roster::addPresence(const QString& account, const QString& jid, const QString& name, const QMap<QString, QVariant>& data)
{
    ElId contactId(account, jid);
    std::multimap<ElId, Contact*>::iterator cBeg = contacts.lower_bound(contactId);
    std::multimap<ElId, Contact*>::iterator cEnd = contacts.upper_bound(contactId);
    for (;cBeg != cEnd; ++cBeg) {
        cBeg->second->addPresence(name, data);
    }
}

void Models::Roster::removePresence(const QString& account, const QString& jid, const QString& name)
{
    ElId contactId(account, jid);
    std::multimap<ElId, Contact*>::iterator cBeg = contacts.lower_bound(contactId);
    std::multimap<ElId, Contact*>::iterator cEnd = contacts.upper_bound(contactId);
    for (;cBeg != cEnd; ++cBeg) {
        cBeg->second->removePresence(name);
    }
}

void Models::Roster::addMessage(const QString& account, const Shared::Message& data)
{
    ElId id(account, data.getPenPalJid());
    
    std::multimap<ElId, Contact*>::iterator cBeg = contacts.lower_bound(id);
    std::multimap<ElId, Contact*>::iterator cEnd = contacts.upper_bound(id);
    
    for (;cBeg != cEnd; ++cBeg) {
        cBeg->second->addMessage(data);
    }
    
    std::map<ElId, Room*>::const_iterator rItr = rooms.find(id);
    if (rItr != rooms.end()) {
        rItr->second->addMessage(data);
    }
}

void Models::Roster::dropMessages(const QString& account, const QString& jid)
{
    ElId id(account, jid);
    for (std::multimap<ElId, Contact*>::iterator cBeg = contacts.lower_bound(id), cEnd = contacts.upper_bound(id) ;cBeg != cEnd; ++cBeg) {
        cBeg->second->dropMessages();
    }
    
    std::map<ElId, Room*>::const_iterator rItr = rooms.find(id);
    if (rItr != rooms.end()) {
        rItr->second->dropMessages();
    }
}

void Models::Roster::removeAccount(const QString& account)
{
    std::map<QString, Account*>::const_iterator itr = accounts.find(account);
    if (itr == accounts.end()) {
        qDebug() << "An attempt to remove non existing account " << account << ", skipping";
        return;
    }
    
    Account* acc = itr->second;
    int index = acc->row();
    root->removeChild(index);
    accountsModel->removeAccount(index);
    accounts.erase(itr);
    
    std::multimap<ElId, Contact*>::const_iterator cItr = contacts.begin();
    while (cItr != contacts.end()) {
        if (cItr->first.account == account) {
            std::multimap<ElId, Contact*>::const_iterator lItr = cItr;
            ++cItr;
            contacts.erase(lItr);
        } else {
            ++cItr;
        }
    }
    
    std::map<ElId, Group*>::const_iterator gItr = groups.begin();
    while (gItr != groups.end()) {
        if (gItr->first.account == account) {
            std::map<ElId, Group*>::const_iterator lItr = gItr;
            ++gItr;
            groups.erase(lItr);
        } else {
            ++gItr;
        }
    }
    
    std::map<ElId, Room*>::const_iterator rItr = rooms.begin();
    while (rItr != rooms.end()) {
        if (rItr->first.account == account) {
            std::map<ElId, Room*>::const_iterator lItr = rItr;
            ++rItr;
            rooms.erase(lItr);
        } else {
            ++rItr;
        }
    }
    
    acc->deleteLater();
}

QString Models::Roster::getContactName(const QString& account, const QString& jid)
{
    ElId id(account, jid);
    std::multimap<ElId, Contact*>::const_iterator cItr = contacts.find(id);
    QString name = "";
    if (cItr == contacts.end()) {
        std::map<ElId, Room*>::const_iterator rItr = rooms.find(id);
        if (rItr == rooms.end()) {
            qDebug() << "An attempt to get a name of non existing contact/room " << account << ":" << jid << ", skipping";
        } else {
            name = rItr->second->getName();
        }
    } else {
        name = cItr->second->getContactName();
    }
    return name;
}

void Models::Roster::addRoom(const QString& account, const QString jid, const QMap<QString, QVariant>& data)
{
    Account* acc;
    {
        std::map<QString, Account*>::iterator itr = accounts.find(account);
        if (itr == accounts.end()) {
            qDebug() << "An attempt to add a room " << jid << " to non existing account " << account << ", skipping";
            return;
        }
        acc = itr->second;
    }
    
    ElId id = {account, jid};
    std::map<ElId, Room*>::const_iterator itr = rooms.find(id);
    if (itr != rooms.end()) {
        qDebug() << "An attempt to add already existing room" << jid << ", skipping";
        return;
    }
    
    Room* room = new Room(jid, data);
    rooms.insert(std::make_pair(id, room));
    acc->appendChild(room);
}

void Models::Roster::changeRoom(const QString& account, const QString jid, const QMap<QString, QVariant>& data)
{
    ElId id = {account, jid};
    std::map<ElId, Room*>::const_iterator itr = rooms.find(id);
    if (itr == rooms.end()) {
        qDebug() << "An attempt to change non existing room" << jid << ", skipping";
        return;
    }
    Room* room = itr->second;
    for (QMap<QString, QVariant>::const_iterator dItr = data.begin(), dEnd = data.end(); dItr != dEnd; ++dItr) {
        room->update(dItr.key(), dItr.value());
    }
}

void Models::Roster::removeRoom(const QString& account, const QString jid)
{
    Account* acc;
    {
        std::map<QString, Account*>::iterator itr = accounts.find(account);
        if (itr == accounts.end()) {
            qDebug() << "An attempt to remove a room " << jid << " from non existing account " << account << ", skipping";
            return;
        }
        acc = itr->second;
    }
    
    ElId id = {account, jid};
    std::map<ElId, Room*>::const_iterator itr = rooms.find(id);
    if (itr == rooms.end()) {
        qDebug() << "An attempt to remove non existing room" << jid << ", skipping";
        return;
    }
    
    Room* room = itr->second;
    acc->removeChild(room->row());
    room->deleteLater();
    rooms.erase(itr);
}
