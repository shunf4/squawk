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

#ifndef CORE_STORAGE_H
#define CORE_STORAGE_H

#include <QString>
#include <lmdb.h>

#include "archive.h"

namespace Core {

/**
 * @todo write docs
 */
class Storage
{
public:
    Storage(const QString& name);
    ~Storage();
    
    void open();
    void close();
    
    void addRecord(const QString& key, const QString& value);
    void removeRecord(const QString& key);
    QString getRecord(const QString& key) const;
    
    
private:
    QString name;
    bool opened;
    MDB_env* environment;
    MDB_dbi base;
};

}

#endif // CORE_STORAGE_H
