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

#ifndef ACCOUNT_H
#define ACCOUNT_H

#include <QDialog>
#include <QScopedPointer>
#include <QMap>
#include <QString>
#include <QVariant>
#include <QStandardItemModel>

#include "shared/global.h"

namespace Ui
{
class Account;
}

class Account : public QDialog
{
    Q_OBJECT

public:
    Account();
    ~Account();
    
    QMap<QString, QVariant> value() const;
    void setData(const QMap<QString, QVariant>& data);
    void lockId();

private slots:
    void onComboboxChange(int index);
    
private:
    QScopedPointer<Ui::Account> m_ui;
};

#endif // ACCOUNT_H
