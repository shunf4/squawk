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

#include "archive.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <QStandardPaths>
#include <QDebug>

Core::Archive::Archive(const QString& p_jid, QObject* parent):
    QObject(parent),
    jid(p_jid),
    opened(false),
    environment(lmdb::env::create()),
    dbi(0)
{
    
}

Core::Archive::~Archive()
{
}

void Core::Archive::open(const QString& account)
{
    if (!opened) {
        QString path(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
        path += "/" + account;
        int state1 = mkdir(path.toStdString().c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        if (state1 != 0 && errno != EEXIST) {
            qDebug() << "Failed to create account " << account << " database folder";
            throw 1;
        }
        
        path += "/" + jid;
        
        int state2 = mkdir(path.toStdString().c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        if (state2 != 0 && errno != EEXIST) {
            qDebug() << "Failed to create " << jid.toStdString().c_str() << " database folder in account" << account;
            throw 1;
        }
        
        environment.set_mapsize(1UL * 1024UL * 1024UL * 1024UL);
        environment.set_max_dbs(10);
        environment.open(path.toStdString().c_str(), 0, 0664);
        
        lmdb::txn wTrans = lmdb::txn::begin(environment);
        dbi = lmdb::dbi::open(wTrans, "main", MDB_CREATE);
        wTrans.commit();
    }
}

QString Core::Archive::addElement(const Shared::Message& message)
{
    
}
