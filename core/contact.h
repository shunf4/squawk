/*
 * <one line to give the program's name and a brief idea of what it does.>
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
#include "archive.h"

namespace Core {

class Contact : public QObject
{
    Q_OBJECT
public:
    enum ArchiveState {
        empty,              //have no messages stored for this contact
        chunk,              //have some chunk of history, don't have the beginning nor have the end
        beginning,          //have the history from the very beginning, don't have the end
        end,                //have the history to the end, but don't have the beginning
        complete            //have full history locally stored
    };
    Contact(const QString& pJid, const QString& account, QObject* parent = 0);
    ~Contact();
    
    ArchiveState getArchiveState() const;
    QString getName() const;
    void setName(const QString& n);
    QSet<QString> getGroups() const;
    
public:
    const QString jid;

private:
    QString name;
    QSet<QString> groups;
    ArchiveState state;
    Archive* archive;
};

}

#endif // CORE_CONTACT_H
