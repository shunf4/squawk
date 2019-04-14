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

#ifndef CORE_ARCHIVE_H
#define CORE_ARCHIVE_H

#include <QObject>
#include "../global.h"
#include "lmdb++.h"

namespace Core {

class Archive : public QObject
{
    Q_OBJECT
public:
    Archive(const QString& jid, QObject* parent);
    ~Archive();
    
    void open(const QString& account);
    
    QString addElement(const Shared::Message& message);
    Shared::Message getElement(const QString& id) const;
    Shared::Message oldest() const;
    Shared::Message newest() const;
    void removeElement(const QString& id);
    void clear();
    void modifyElement(const QString& id, const Shared::Message& newValue);
    unsigned int size() const;
    
public:
    const QString jid;
    
private:
    bool opened;
    lmdb::env environment;
    lmdb::dbi dbi;
};

}

#endif // CORE_ARCHIVE_H
