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
}

Models::Roster::~Roster()
{
    delete accountsModel;
    delete root;
}

void Models::Roster::addAccount(const QMap<QString, QVariant>& data)
{
    Account* acc = new Account(data, root);
    beginInsertRows(QModelIndex(), root->childCount(), root->childCount());
    root->appendChild(acc);
    accounts.insert(std::make_pair(acc->getName(), acc));
    accountsModel->addAccount(acc);
    endInsertRows();
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
            result = item->data(index.column());
        }
            break;
        case Qt::DecorationRole:
            switch (item->type) {
                case Item::account:{
                    Account* acc = static_cast<Account*>(item);
                    int state = acc->getState();
                    switch (state) {
                        case Shared::disconnected:
                            result = QIcon::fromTheme("im-user-offline");
                            break;
                        case Shared::connecting:
                            result = QIcon::fromTheme(Shared::ConnectionStateThemeIcons[state]);
                            break;
                        case Shared::connected:
                            result = QIcon::fromTheme("im-user-online");
                            break;
                    }
                }
                    break;
                case Item::contact:{
                    Contact* contact = static_cast<Contact*>(item);
                    int state = contact->getState();
                    switch (state) {
                        case 0:
                            result = QIcon::fromTheme("im-user-offline");
                            break;
                        case 1:
                            result = QIcon::fromTheme("im-user-online");
                            break;
                    }
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
        return QModelIndex();
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
    std::map<ElId, Item*>::const_iterator gItr = groups.find(id);
    if (gItr != groups.end()) {
        qDebug() << "An attempt to add group " << name << " to account " << account <<" which already exists there, skipping";
        return;
    }
    std::map<QString, Account*>::iterator itr = accounts.find(account);
    if (itr != accounts.end()) {
        Account* acc = itr->second;
        Item* group = new Item(Item::group, {{"name", name}}, acc);
        beginInsertRows(createIndex(acc->row(), 0, acc), acc->childCount(), acc->childCount());
        acc->appendChild(group);
        groups.insert(std::make_pair(id, group));
        endInsertRows();
    } else {
        qDebug() << "An attempt to add group " << name << " to non existing account " << account << ", skipping";
    }
}

void Models::Roster::addContact(const QString& account, const QString& jid, const QString& name, const QString& group)
{
    Item* parent;
    Account* acc;
    Contact* contact;
    ElId id(account, jid);
    
    {
        std::map<QString, Account*>::iterator itr = accounts.find(account);
        if (itr == accounts.end()) {
            qDebug() << "An attempt to add a contact " << name << " to non existing account " << account << ", skipping";
            return;
        }
        acc = itr->second;
    }
    
    if (group == "") {
        std::multimap<ElId, Contact*>::iterator itr = contacts.lower_bound(id);
        std::multimap<ElId, Contact*>::iterator eItr = contacts.upper_bound(id);
        while (itr != eItr) {
            if (itr->second->parentItem() == acc) {
                qDebug() << "An attempt to add a contact " << name << " ungrouped to non the account " << account << " for the second time, skipping";
                return;
            }
        }
        parent = acc;
    } else {
        std::map<ElId, Item*>::iterator itr = groups.find({account, group});
        if (itr == groups.end()) {
            qDebug() << "An attempt to add a contact " << name << " to non existing group " << group << ", skipping";
            return;
        }
        
        parent = itr->second;
        
        for (int i = 0; i < parent->childCount(); ++i) {
            Item* item = parent->child(i);
            if (item->type == Item::contact) {
                Contact* ca = static_cast<Contact*>(item);
                if (ca->getJid() == jid) {
                    qDebug() << "An attempt to add a contact " << name << " to the group " << group << " for the second time, skipping";
                    return;
                }
            }
        }
        
        for (int i = 0; i < acc->childCount(); ++i) {
            Item* item = acc->child(i);
            if (item->type == Item::contact) {
                Contact* ca = static_cast<Contact*>(item);
                if (ca->getJid() == jid) {
                    qDebug() << "An attempt to add a already existing contact " << name << " to the group " << group << ", contact will be moved from ungrouped contacts of " << account;
                    
                    beginMoveRows(createIndex(acc->row(), 0, acc), i, i, createIndex(parent->row(), 0, parent), parent->childCount());
                    contact = ca;
                    acc->removeChild(i);
                    ca->setParent(parent);
                    parent->appendChild(ca);
                    endMoveRows();
                    return;
                }
            }
        }
        
    }
    contact = new Contact({{"name", name}, {"jid", jid}, {"state", 0}}, parent);
    beginInsertRows(createIndex(parent->row(), 0, parent), parent->childCount(), parent->childCount());
    parent->appendChild(contact);
    contacts.insert(std::make_pair(id, contact));
    endInsertRows();
    
}

void Models::Roster::removeGroup(const QString& account, const QString& name)
{
    ElId id(account, name);
    std::map<ElId, Item*>::const_iterator gItr = groups.find(id);
    if (gItr == groups.end()) {
        qDebug() << "An attempt to remove group " << name << " from account " << account <<" which doesn't exist there, skipping";
        return;
    }
    Item* item = gItr->second;
    Item* parent = item->parentItem();
    int row = item->row();
    
    beginRemoveRows(createIndex(parent->row(), 0, parent), row, row);
    parent->removeChild(row);
    endRemoveRows();
    
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
        beginInsertRows(createIndex(acc->row(), 0, acc), acc->childCount(), acc->childCount() + toInsert.size() - 1);
        for (int i = 0; i < toInsert.size(); ++i) {
            Contact* cont = toInsert[i];
            cont->setParent(acc);
            acc->appendChild(cont);
        }
        endInsertRows();
    }
    
    delete item;
}

void Models::Roster::changeContact(const QString& account, const QString& jid, const QString& name)
{
    ElId id(account, jid);
    std::multimap<ElId, Contact*>::iterator cBeg = contacts.lower_bound(id);
    std::multimap<ElId, Contact*>::iterator cEnd = contacts.upper_bound(id);
    
    for (; cBeg != cEnd; ++cBeg) {
        cBeg->second->setName(name);
    }
}

void Models::Roster::removeContact(const QString& account, const QString& jid)
{
    ElId id(account, jid);
    std::multimap<ElId, Contact*>::iterator cBeg = contacts.lower_bound(id);
    std::multimap<ElId, Contact*>::iterator cEnd = contacts.upper_bound(id);
    
    QSet<QString> toRemove;
    for (; cBeg != cEnd; ++cBeg) {
        Contact* contact = cBeg->second;
        Item* parent = contact->parentItem();
        if (parent->type == Item::group && parent->childCount() == 1) {
            toRemove.insert(parent->getName());
        }
        int row = contact->row();
        beginRemoveRows(createIndex(parent->row(), 0, parent), row, row);
        parent->removeChild(row);
        endRemoveRows();
        delete contact;
    }
    
    for (QSet<QString>::const_iterator itr = toRemove.begin(), end = toRemove.end(); itr != end; ++itr) {
        removeGroup(account, *itr);
    }
}

void Models::Roster::removeContact(const QString& account, const QString& jid, const QString& group)
{
    ElId contactId(account, jid);
    ElId groupId(account, group);
    
    std::map<ElId, Item*>::iterator gItr = groups.find(groupId);
    if (gItr == groups.end()) {
        qDebug() << "An attempt to remove contact " << jid << " from non existing group " << group << " of account " << account <<", skipping";
        return;
    }
    Item* gr = gItr->second;
    Contact* cont = 0;
    
    std::multimap<ElId, Contact*>::iterator cBeg = contacts.lower_bound(contactId);
    std::multimap<ElId, Contact*>::iterator cEnd = contacts.upper_bound(contactId);
    for (;cBeg != cEnd; ++cBeg) {
        if (cBeg->second->parentItem() == gr) {
            cont = cBeg->second;
            break;
        }
    }
    
    if (cont == 0) {
        qDebug() << "An attempt to remove contact " << jid << " of account " << account << " from group " << group  <<", but there is no such contact in that group, skipping";
        return;
    }
    
    int row = cont->row();
    beginRemoveRows(createIndex(gr->row(), 0, gr), row, row);
    gr->removeChild(row);
    endRemoveRows();
    delete cont;
    
    if (gr->childCount() == 0) {
        removeGroup(account, group);
    }
}