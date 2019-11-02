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

#include "comboboxdelegate.h"

ComboboxDelegate::ComboboxDelegate(QObject *parent):
    QStyledItemDelegate(parent),
    entries()
{
}


ComboboxDelegate::~ComboboxDelegate()
{
}


QWidget* ComboboxDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QComboBox *cb = new QComboBox(parent);
    
    for (const std::pair<QString, QIcon> pair : entries) {
        cb->addItem(pair.second, pair.first);
    }
    
    return cb;
}


void ComboboxDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QComboBox *cb = static_cast<QComboBox*>(editor);
    int currentIndex = index.data(Qt::EditRole).toInt();
    if (currentIndex >= 0) {
        cb->setCurrentIndex(currentIndex);
    }
}


void ComboboxDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    QComboBox *cb = static_cast<QComboBox *>(editor);
    model->setData(index, cb->currentIndex(), Qt::EditRole);
}

void ComboboxDelegate::addEntry(const QString& title, const QIcon& icon)
{
    entries.emplace_back(title, icon);
}
