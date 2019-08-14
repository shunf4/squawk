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

#include "contact.h"
#include <QDebug>

Core::Contact::Contact(const QString& pJid, const QString& account, QObject* parent):
    RosterItem(pJid, account, parent),
    groups()
{
}

Core::Contact::~Contact()
{
}

QSet<QString> Core::Contact::getGroups() const
{
    return groups;
}

unsigned int Core::Contact::groupsCount() const
{
    return groups.size();
}

void Core::Contact::setGroups(const QSet<QString>& set)
{
    QSet<QString> toRemove = groups - set;
    QSet<QString> toAdd = set - groups;
    
    groups = set;
    
    for (QSet<QString>::iterator itr = toRemove.begin(), end = toRemove.end(); itr != end; ++itr) {
        emit groupRemoved(*itr);
    }
    
    for (QSet<QString>::iterator itr = toAdd.begin(), end = toAdd.end(); itr != end; ++itr) {
        emit groupAdded(*itr);
    }
}
