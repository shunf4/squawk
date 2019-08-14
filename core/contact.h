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

#ifndef CORE_CONTACT_H
#define CORE_CONTACT_H

#include <QObject>
#include <QSet>
#include "rosteritem.h"

namespace Core {

class Contact : public RosterItem
{
    Q_OBJECT
public:
    Contact(const QString& pJid, const QString& account, QObject* parent = 0);
    ~Contact();

    QSet<QString> getGroups() const;
    void setGroups(const QSet<QString>& set);
    unsigned int groupsCount() const;

signals:
    void groupAdded(const QString& name);
    void groupRemoved(const QString& name);

private:
    QSet<QString> groups;
};
}

#endif // CORE_CONTACT_H
