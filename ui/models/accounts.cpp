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

#include "accounts.h"
#include "shared/icons.h"

#include <QIcon>
#include <QDebug>

std::deque<QString> Models::Accounts::columns = {"Name", "Server", "State", "Error"};

Models::Accounts::Accounts(QObject* parent):
    QAbstractTableModel(parent),
    accs()
{

}

Models::Accounts::~Accounts()
{

}

QVariant Models::Accounts::data (const QModelIndex& index, int role) const
{
    QVariant answer;
    switch (role) {
        case Qt::DisplayRole: 
            answer = accs[index.row()]->data(index.column());
            break;
        case Qt::DecorationRole:
            if (index.column() == 2) {
                answer = Shared::connectionStateIcon(accs[index.row()]->getState());
            }
            break;
        default:
            break;
    }
    
    return answer;
}

int Models::Accounts::columnCount ( const QModelIndex& parent ) const
{
    return columns.size();
}

int Models::Accounts::rowCount ( const QModelIndex& parent ) const
{
    return accs.size();
}

QVariant Models::Accounts::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        return tr(columns[section].toLatin1());
    }
    return QVariant();
}


void Models::Accounts::addAccount(Account* account)
{
    beginInsertRows(QModelIndex(), accs.size(), accs.size());
    int index = 0;
    std::deque<Account*>::const_iterator before = accs.begin();
    while (before != accs.end()) {
        Account* bfr = *before;
        if (bfr->getDisplayedName() > account->getDisplayedName()) {
            break;
        }
        index++;
        before++;
    }
    
    accs.insert(before, account);
    connect(account, &Account::childChanged, this, &Accounts::onAccountChanged);
    endInsertRows();
    
    emit sizeChanged(accs.size());
}

void Models::Accounts::onAccountChanged(Item* item, int row, int col)
{
    if (row < accs.size()) {
        Account* acc = getAccount(row);
        if (item != acc) {
            return;     //it means the signal is emitted by one of accounts' children, not exactly him, this model has no interest in that
        }
        
        if (col == 0) {
            int newRow = 0;
            std::deque<Account*>::const_iterator before = accs.begin();
            while (before != accs.end()) {
                Item* bfr = *before;
                if (bfr->getDisplayedName() > item->getDisplayedName()) {
                    break;
                }
                newRow++;
                before++;
            }
            
            if (newRow != row || (before != accs.end() && *before != item)) {
                emit beginMoveRows(createIndex(row, 0), row, row, createIndex(newRow, 0), newRow);
                std::deque<Account*>::const_iterator old = accs.begin();
                old += row;
                accs.erase(old);
                accs.insert(before, acc);
                emit endMoveRows();
                
                row = newRow;
            }
        }
        
        if (col < columnCount(QModelIndex())) {
            emit dataChanged(createIndex(row, col), createIndex(row, col));
        }
        emit changed();
    }
}

Models::Account * Models::Accounts::getAccount(int index)
{
    return accs[index];
}

void Models::Accounts::removeAccount(int index)
{
    Account* account = accs[index];
    beginRemoveRows(QModelIndex(), index, index);
    disconnect(account, &Account::childChanged, this, &Accounts::onAccountChanged);
    accs.erase(accs.begin() + index);
    endRemoveRows();
    
    emit sizeChanged(accs.size());
}

std::deque<QString> Models::Accounts::getNames() const
{
    std::deque<QString> res;
    
    for (std::deque<Models::Account*>::const_iterator i = accs.begin(), end = accs.end(); i != end; ++i) {
        res.push_back((*i)->getName());
    }
    
    return res;
}
