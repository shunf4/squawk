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

#ifndef MODELS_ACCOUNTS_H
#define MODELS_ACCOUNTS_H

#include <qabstractitemmodel.h>
#include <deque>
#include "account.h"

namespace Models
{

class Accounts : public QAbstractTableModel
{
    Q_OBJECT
public:
    Accounts(QObject* parent = 0);
    ~Accounts();
    
    void addAccount(Account* account);
    void removeAccount(int index);
    
    QVariant data ( const QModelIndex& index, int role ) const override;
    int columnCount ( const QModelIndex& parent ) const override;
    int rowCount ( const QModelIndex& parent ) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    Account* getAccount(int index);
    
    std::deque<QString> getNames() const;
    
signals:
    void changed();
    void sizeChanged(unsigned int size);
    
private:
    std::deque<Account*> accs;
    static std::deque<QString> columns;
    
private slots:
    void onAccountChanged(Models::Item* item, int row, int col);
    
};
}

#endif // MODELS_ACCOUNT_H
