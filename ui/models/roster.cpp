#include "roster.h"
#include <QDebug>
#include <QIcon>

using namespace Models;

Models::Roster::Roster(QObject* parent):
    QAbstractItemModel(parent),
    accountsModel(new Accounts()),
    root(new Item(Item::root, {{"name", "root"}})),
    accounts(),
    groups(),
    contacts(),
    elements()
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
                default:
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
    std::map<QString, Account*>::iterator itr = accounts.find(account);
    if (itr != accounts.end()) {
        Account* acc = itr->second;
        Item* group = new Item(Item::group, {{"name", name}}, acc);
        beginInsertRows(createIndex(acc->row(), 0, acc), acc->childCount(), acc->childCount());
        acc->appendChild(group);
        groups.insert(std::make_pair(name, group));
        elements.insert({{account, name}, group});
        endInsertRows();
    } else {
        qDebug() << "An attempt to add group " << name << " to non existing account " << account << ", skipping";
    }
}

void Models::Roster::addContact(const QString& account, const QString& jid, const QString& name, const QString& group)
{
    Item* parent;
    if (group == "") {
        std::map<QString, Account*>::iterator itr = accounts.find(account);
        if (itr == accounts.end()) {
            qDebug() << "An attempt to add a contact " << name << " to non existing account " << account << ", skipping";
            return;
        }
        parent = itr->second;
    } else {
        std::map<QString, Item*>::iterator itr = groups.find(group);
        if (itr == groups.end()) {
            qDebug() << "An attempt to add a contact " << name << " to non existing group " << group << ", skipping";
            return;
        }
        parent = itr->second;
    }
    
    QString sName = name;
    if (sName == "") {
        sName = jid;
    }
    Item* contact = new Item(Item::contact, {{"name", sName}}, parent);
    beginInsertRows(createIndex(parent->row(), 0, parent), parent->childCount(), parent->childCount());
    parent->appendChild(contact);
    contacts.insert(std::make_pair(jid, contact));
    elements.insert({{account, jid}, contact});
    endInsertRows();
}

void Models::Roster::removeGroup(const QString& account, const QString& name)
{
}

