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

#include "emailsmodel.h"

UI::VCard::EMailsModel::EMailsModel(bool p_edit, QObject* parent):
    QAbstractTableModel(parent),
    edit(p_edit),
    deque()
{
}

int UI::VCard::EMailsModel::columnCount(const QModelIndex& parent) const
{
    return 3;
}

int UI::VCard::EMailsModel::rowCount(const QModelIndex& parent) const
{
    return deque.size();
}

QVariant UI::VCard::EMailsModel::data(const QModelIndex& index, int role) const
{
    if (index.isValid()) {
        int col = index.column();
        switch (col) {
            case 0:
                switch (role) {
                    case Qt::DisplayRole:
                    case Qt::EditRole:
                        return deque[index.row()].address;
                    default:
                        return QVariant();
                }
                break;
            case 1:
                switch (role) {
                    case Qt::DisplayRole:
                        return tr(Shared::VCard::Email::roleNames[deque[index.row()].role].toStdString().c_str());
                    case Qt::EditRole: 
                        return deque[index.row()].role;
                    default:
                        return QVariant();
                }
                break;
            case 2:
                switch (role) {
                    case Qt::DisplayRole:
                        return QVariant();
                    case Qt::DecorationRole:
                        if (deque[index.row()].prefered) {
                            return Shared::icon("favorite", false);
                        }
                        return QVariant();
                    default:
                        return QVariant();
                }
                break;
            default:
                return QVariant();
        }
    }
    return QVariant();
}

Qt::ItemFlags UI::VCard::EMailsModel::flags(const QModelIndex& index) const
{
    Qt::ItemFlags f = QAbstractTableModel::flags(index);
    if (edit && index.column() != 2) {
        f = Qt::ItemIsEditable | f;
    }
    return  f;
}

bool UI::VCard::EMailsModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (role == Qt::EditRole && checkIndex(index)) {
        Shared::VCard::Email& item = deque[index.row()];
        switch (index.column()) {
            case 0:
                item.address = value.toString();
                return true;
            case 1: {
                quint8 newRole = value.toUInt();
                if (newRole > Shared::VCard::Email::work) {
                    return false;
                }
                item.role = static_cast<Shared::VCard::Email::Role>(newRole);
                return true;
            }
            case 2: {
                bool newDef = value.toBool();
                if (newDef != item.prefered) {
                    if (newDef) {
                        //dropPrefered();
                    }
                    item.prefered = newDef;
                    return true;
                }
            }
        }
        return true;
    }
    return false;
}


bool UI::VCard::EMailsModel::dropPrefered()
{
    bool dropped = false;
    int i = 0;
    for (Shared::VCard::Email& email : deque) {
        if (email.prefered) {
            email.prefered = false;
            QModelIndex ci = createIndex(i, 2, &email);
            emit dataChanged(ci, ci);
            dropped = true;
        }
        ++i;
    }
    return dropped;
}

QModelIndex UI::VCard::EMailsModel::addNewEmptyLine()
{
    beginInsertRows(QModelIndex(), deque.size(), deque.size());
    deque.emplace_back("");
    endInsertRows();
    return createIndex(deque.size() - 1, 0, &(deque.back()));
}

bool UI::VCard::EMailsModel::isPreferred(int row) const
{
    if (row < deque.size()) {
        return deque[row].prefered;
    } else {
        return false;
    }
}

void UI::VCard::EMailsModel::removeLines(int index, int count)
{
    if (index < deque.size()) {
        int maxCount = deque.size() - index;
        if (count > maxCount) {
            count = maxCount;
        }
        
        if (count > 0) {
            beginRemoveRows(QModelIndex(), index, index + count - 1);
            std::deque<Shared::VCard::Email>::const_iterator itr = deque.begin() + index;
            std::deque<Shared::VCard::Email>::const_iterator end = itr + count;
            deque.erase(itr, end);
            endRemoveRows();
        }
    }
}

void UI::VCard::EMailsModel::getEmails(std::deque<Shared::VCard::Email>& emails) const
{
    for (const Shared::VCard::Email& my : deque) {
        emails.emplace_back(my);
    }
}

void UI::VCard::EMailsModel::setEmails(const std::deque<Shared::VCard::Email>& emails)
{
    if (deque.size() > 0) {
        removeLines(0, deque.size());
    }
    
    if (emails.size() > 0) {
        beginInsertRows(QModelIndex(), 0, emails.size() - 1);
        for (const Shared::VCard::Email& comming : emails) {
            deque.emplace_back(comming);
        }
        endInsertRows();
    }
}

void UI::VCard::EMailsModel::revertPreferred(int row)
{
    setData(createIndex(row, 2), !isPreferred(row));
}
