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

#ifndef JOINCONFERENCE_H
#define JOINCONFERENCE_H

#include <QDialog>
#include <QScopedPointer>

#include "../models/accounts.h"

namespace Ui
{
class JoinConference;
}

/**
 * @todo write docs
 */
class JoinConference : public QDialog
{
    Q_OBJECT
public:
    struct Data {
        QString jid;
        QString nick;
        QString account;
        bool autoJoin;
        QString password;
    };
    
    JoinConference(const Models::Accounts* accounts, QWidget* parent = 0);
    JoinConference(const QString& acc, const Models::Accounts* accounts, QWidget* parent = 0);
    ~JoinConference();
    
    Data value() const;

private:
    QScopedPointer<Ui::JoinConference> m_ui;
};

#endif // JOINCONFERENCE_H
