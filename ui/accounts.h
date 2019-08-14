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

#ifndef ACCOUNTS_H
#define ACCOUNTS_H

#include <QWidget>
#include <QScopedPointer>
#include <QItemSelection>

#include "account.h"
#include "models/accounts.h"

namespace Ui
{
class Accounts;
}

class Accounts : public QWidget
{
    Q_OBJECT
public:
    explicit Accounts(Models::Accounts* p_model, QWidget *parent = nullptr);
    ~Accounts() override;
    
signals:
    void newAccount(const QMap<QString, QVariant>&);
    void changeAccount(const QString&, const QMap<QString, QVariant>&);
    void connectAccount(const QString&);
    void disconnectAccount(const QString&);
    void removeAccount(const QString&);
    
private slots:
    void onAddButton(bool clicked = 0);
    void onEditButton(bool clicked = 0);
    void onConnectButton(bool clicked = 0);
    void onDeleteButton(bool clicked = 0);
    void onAccountAccepted();
    void onAccountRejected();
    void onSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
    void updateConnectButton();
    
private:
    QScopedPointer<Ui::Accounts> m_ui;
    Models::Accounts* model;
    bool editing;
    bool toDisconnect;
};

#endif // ACCOUNTS_H
