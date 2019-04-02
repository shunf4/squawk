#include "roster.h"
#include <QIcon>

using namespace Models;

Models::Roster::Roster(QObject* parent):
    QAbstractItemModel(parent),
    root(0)
{
    root = new Item(Item::root, {{"name", "root"}});
}

Models::Roster::~Roster()
{
    delete root;
}

void Models::Roster::addAccount(const QMap<QString, QVariant>& data)
{
    Account* acc = new Account(data, root);
    beginInsertRows(QModelIndex(), root->childCount(), root->childCount());
    root->appendChild(acc);
    accounts.insert(std::make_pair(acc->name(), acc));
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
                    int state = item->data(1).toInt();
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

void Models::Roster::Item::setName(const QString& name)
{
    itemData[0] = name;
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
        if (field == "name") {
            acc->setName(value.toString());
            accounts.erase(itr);
            accounts.insert(std::make_pair(acc->name(), acc));
            int row = acc->row();
            emit dataChanged(createIndex(row, 0, acc), createIndex(row, 0, acc));
        } else if (field == "state") {
            acc->setState(value.toInt());
            int row = acc->row();
            emit dataChanged(createIndex(row, 0, acc), createIndex(row, 0, acc));
        }
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


Models::Roster::Item::Item(Type p_type, const QMap<QString, QVariant> &p_data, Item *p_parent):
    type(p_type),
    childItems(),
    itemData(),
    parent(p_parent)
{
    itemData.push_back(p_data.value("name"));
}

Models::Roster::Item::~Item()
{
    std::deque<Item*>::const_iterator itr = childItems.begin();
    std::deque<Item*>::const_iterator end = childItems.end();
    
    for (;itr != end; ++itr) {
        delete (*itr);
    }
}

void Models::Roster::Item::appendChild(Models::Roster::Item* child)
{
    childItems.push_back(child);
}

Models::Roster::Item * Models::Roster::Item::child(int row)
{
    return childItems[row];
}

int Models::Roster::Item::childCount() const
{
    return childItems.size();
}

int Models::Roster::Item::row() const
{
    if (parent != 0) {
        std::deque<Item*>::const_iterator itr = parent->childItems.begin();
        std::deque<Item*>::const_iterator end = parent->childItems.end();
        
        for (int i = 0; itr != end; ++itr, ++i) {
            if (*itr == this) {
                return i;
            }
        }
    }
    
    return 0;       //TODO not sure how it helps, i copy-pasted it from the example
}

Models::Roster::Item * Models::Roster::Item::parentItem()
{
    return parent;
}

int Models::Roster::Item::columnCount() const
{
    return itemData.size();
}

QString Models::Roster::Item::name() const
{
    return itemData[0].toString();
}


QVariant Models::Roster::Item::data(int column) const
{
    return itemData[column];
}

Models::Roster::ElId::ElId(const QString& p_account, const QString& p_name):
    account(p_account),
    name(p_name)
{
    
}

bool Models::Roster::ElId::operator <(const Models::Roster::ElId& other) const
{
    if (account == other.account) {
        return name < other.name;
    } else {
        return account < other.account;
    }
}

Models::Roster::Account::Account(const QMap<QString, QVariant>& data, Models::Roster::Item* parentItem):
    Item(account, data, parentItem)
{
    itemData.push_back(data.value("state"));
}

Models::Roster::Account::~Account()
{
}

void Models::Roster::Account::setState(int state)
{
    itemData[1] = state;
}


