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

#include "account.h"
#include "ui_account.h"

Account::Account(): 
    QDialog(),
    m_ui(new Ui::Account)
{
    m_ui->setupUi(this);
    
    connect(m_ui->passwordType, qOverload<int>(&QComboBox::currentIndexChanged), this, &Account::onComboboxChange);
    
    for (int i = static_cast<int>(Shared::AccountPasswordLowest); i < static_cast<int>(Shared::AccountPasswordHighest) + 1; ++i) {
        Shared::AccountPassword ap = static_cast<Shared::AccountPassword>(i);
        m_ui->passwordType->addItem(Shared::Global::getName(ap));
    }
    m_ui->passwordType->setCurrentIndex(static_cast<int>(Shared::AccountPassword::plain));
    
    if (!Shared::Global::supported("KWallet")) {
        QStandardItemModel *model = static_cast<QStandardItemModel*>(m_ui->passwordType->model());
        QStandardItem *item = model->item(static_cast<int>(Shared::AccountPassword::kwallet));
        item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
    }
}

Account::~Account()
{
}

QMap<QString, QVariant> Account::value() const
{
    QMap<QString, QVariant> map;
    map["login"] = m_ui->login->text();
    map["password"] = m_ui->password->text();
    map["server"] = m_ui->server->text();
    map["name"] = m_ui->name->text();
    map["resource"] = m_ui->resource->text();
    map["passwordType"] = m_ui->passwordType->currentIndex();
    
    return map;
}

void Account::lockId()
{
    m_ui->name->setReadOnly(true);;
}

void Account::setData(const QMap<QString, QVariant>& data)
{
    m_ui->login->setText(data.value("login").toString());
    m_ui->password->setText(data.value("password").toString());
    m_ui->server->setText(data.value("server").toString());
    m_ui->name->setText(data.value("name").toString());
    m_ui->resource->setText(data.value("resource").toString());
    m_ui->passwordType->setCurrentIndex(data.value("passwordType").toInt());
}

void Account::onComboboxChange(int index)
{
    QString description = Shared::Global::getDescription(Shared::Global::fromInt<Shared::AccountPassword>(index));
    m_ui->comment->setText(description);
}
