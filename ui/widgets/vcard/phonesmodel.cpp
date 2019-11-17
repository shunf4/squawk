/*
 * Squawk messenger. 
 * Copyright (C) 2019 Yury Gubich <blue@macaw.me>
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

#include "phonesmodel.h"

UI::VCard::PhonesModel::PhonesModel(bool p_edit, QObject* parent):
    QAbstractTableModel(parent),
    edit(p_edit),
    deque()
{
}

int UI::VCard::PhonesModel::columnCount(const QModelIndex& parent) const
{
    return 4;
}

int UI::VCard::PhonesModel::rowCount(const QModelIndex& parent) const
{
    return deque.size();
}

QVariant UI::VCard::PhonesModel::data(const QModelIndex& index, int role) const
{
    if (index.isValid()) {
        int col = index.column();
        switch (col) {
            case 0:
                switch (role) {
                    case Qt::DisplayRole:
                    case Qt::EditRole:
                        return deque[index.row()].number;
                    default:
                        return QVariant();
                }
                break;
            case 1:
                switch (role) {
                    case Qt::DisplayRole:
                        return QCoreApplication::translate("Global", Shared::VCard::Phone::roleNames[deque[index.row()].role].toStdString().c_str());
                    case Qt::EditRole: 
                        return deque[index.row()].role;
                    default:
                        return QVariant();
                }
                break;
            case 2:
                switch (role) {
                    case Qt::DisplayRole:
                        return QCoreApplication::translate("Global", Shared::VCard::Phone::typeNames[deque[index.row()].type].toStdString().c_str());
                    case Qt::EditRole: 
                        return deque[index.row()].type;
                    default:
                        return QVariant();
                }
                break;
            case 3:
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

QModelIndex UI::VCard::PhonesModel::addNewEmptyLine()
{
    beginInsertRows(QModelIndex(), deque.size(), deque.size());
    deque.emplace_back("", Shared::VCard::Phone::other);
    endInsertRows();
    return createIndex(deque.size() - 1, 0, &(deque.back()));
}

Qt::ItemFlags UI::VCard::PhonesModel::flags(const QModelIndex& index) const
{
    Qt::ItemFlags f = QAbstractTableModel::flags(index);
    if (edit && index.column() != 3) {
        f = Qt::ItemIsEditable | f;
    }
    return  f;
}

bool UI::VCard::PhonesModel::dropPrefered()
{
    bool dropped = false;
    int i = 0;
    for (Shared::VCard::Phone& phone : deque) {
        if (phone.prefered) {
            phone.prefered = false;
            QModelIndex ci = createIndex(i, 2, &phone);
            emit dataChanged(ci, ci);
            dropped = true;
        }
        ++i;
    }
    return dropped;
}

void UI::VCard::PhonesModel::getPhones(std::deque<Shared::VCard::Phone>& phones) const
{
    for (const Shared::VCard::Phone& my : deque) {
        phones.emplace_back(my);
    }
}

bool UI::VCard::PhonesModel::isPreferred(quint32 row) const
{
    if (row < deque.size()) {
        return deque[row].prefered;
    } else {
        return false;
    }
}

void UI::VCard::PhonesModel::removeLines(quint32 index, quint32 count)
{
    if (index < deque.size()) {
        quint32 maxCount = deque.size() - index;
        if (count > maxCount) {
            count = maxCount;
        }
        
        if (count > 0) {
            beginRemoveRows(QModelIndex(), index, index + count - 1);
            std::deque<Shared::VCard::Phone>::const_iterator itr = deque.begin() + index;
            std::deque<Shared::VCard::Phone>::const_iterator end = itr + count;
            deque.erase(itr, end);
            endRemoveRows();
        }
    }
}

void UI::VCard::PhonesModel::revertPreferred(quint32 row)
{
    setData(createIndex(row, 3), !isPreferred(row));
}

bool UI::VCard::PhonesModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (role == Qt::EditRole && checkIndex(index)) {
        Shared::VCard::Phone& item = deque[index.row()];
        switch (index.column()) {
            case 0:
                item.number = value.toString();
                return true;
            case 1: {
                quint8 newRole = value.toUInt();
                if (newRole > Shared::VCard::Phone::work) {
                    return false;
                }
                item.role = static_cast<Shared::VCard::Phone::Role>(newRole);
                return true;
            }
            case 2: {
                quint8 newType = value.toUInt();
                if (newType > Shared::VCard::Phone::other) {
                    return false;
                }
                item.type = static_cast<Shared::VCard::Phone::Type>(newType);
                return true;
            }
            case 3: {
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

void UI::VCard::PhonesModel::setPhones(const std::deque<Shared::VCard::Phone>& phones)
{
    if (deque.size() > 0) {
        removeLines(0, deque.size());
    }
    
    if (phones.size() > 0) {
        beginInsertRows(QModelIndex(), 0, phones.size() - 1);
        for (const Shared::VCard::Phone& comming : phones) {
            deque.emplace_back(comming);
        }
        endInsertRows();
    }
}

QString UI::VCard::PhonesModel::getPhone(quint32 row) const
{
    return deque[row].number;
}
