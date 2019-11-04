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

#ifndef UI_VCARD_EMAILSMODEL_H
#define UI_VCARD_EMAILSMODEL_H

#include <QAbstractTableModel>
#include <QIcon>

#include <deque>

#include "global.h"

namespace UI {
namespace VCard {

class EMailsModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    EMailsModel(bool edit = false, QObject *parent = nullptr);
    
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool isPreferred(int row) const;
    
    void removeLines(int index, int count);
    void setEmails(const std::deque<Shared::VCard::Email>& emails);
    void getEmails(std::deque<Shared::VCard::Email>& emails) const;
    
public slots:
    QModelIndex addNewEmptyLine();
    void revertPreferred(int row);
    
private:
    bool edit;
    std::deque<Shared::VCard::Email> deque;
    
private:
    bool dropPrefered();
};

}}

#endif // UI_VCARD_EMAILSMODEL_H
