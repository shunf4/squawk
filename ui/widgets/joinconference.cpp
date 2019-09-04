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

#include "joinconference.h"
#include "ui_joinconference.h"

#include <QDebug>

JoinConference::JoinConference(const Models::Accounts* accounts, QWidget* parent):
    QDialog(parent),
    m_ui(new Ui::JoinConference())
{
    m_ui->setupUi ( this );
    std::deque<QString> names = accounts->getNames();
    
    for (std::deque<QString>::const_iterator i = names.begin(), end = names.end(); i != end; i++) {
        m_ui->account->addItem(*i);
    }
    
    m_ui->account->setCurrentIndex(0);
}

JoinConference::JoinConference(const QString& acc, const Models::Accounts* accounts, QWidget* parent):
    QDialog(parent),
    m_ui(new Ui::JoinConference())
{
    m_ui->setupUi ( this );
    std::deque<QString> names = accounts->getNames();
    
    int index = 0;
    bool found = false;
    for (std::deque<QString>::const_iterator i = names.begin(), end = names.end(); i != end; i++) {
        const QString& name = *i;
        m_ui->account->addItem(name);
        if (!found) {
            if (name == acc) {
                found = true;
            } else {
                index++;
            }
        }
    }
    
    if (!found) {
        qDebug() << "Couldn't find a correct account among available accounts creating JoinConference dialog, setting to 0";
    }
    
    m_ui->account->setCurrentIndex(index);
}

JoinConference::~JoinConference()
{
}

JoinConference::Data JoinConference::value() const
{
    return {
        m_ui->jid->text(),
        m_ui->nick->text(),
        m_ui->account->currentText(),
        m_ui->autoJoin->isChecked(),
        ""
    };
}
