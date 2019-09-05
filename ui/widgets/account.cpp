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

Account::Account()
    : m_ui ( new Ui::Account )
{
    m_ui->setupUi ( this );
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
}