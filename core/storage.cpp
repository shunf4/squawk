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

#include <QStandardPaths>
#include <QDir>

#include "storage.h"

Core::Storage::Storage(const QString& p_name):
    name(p_name),
    opened(false),
    environment(),
    base()
{
}

Core::Storage::~Storage()
{
    close();
}

void Core::Storage::open()
{
    if (!opened) {
        mdb_env_create(&environment);
        QString path(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
        path += "/" + name;
        QDir cache(path);
        
        if (!cache.exists()) {
            bool res = cache.mkpath(path);
            if (!res) {
                throw Archive::Directory(path.toStdString());
            }
        }
        
        mdb_env_set_maxdbs(environment, 1);
        mdb_env_set_mapsize(environment, 10UL * 1024UL * 1024UL);
        mdb_env_open(environment, path.toStdString().c_str(), 0, 0664);
        
        MDB_txn *txn;
        mdb_txn_begin(environment, NULL, 0, &txn);
        mdb_dbi_open(txn, "base", MDB_CREATE, &base);
        mdb_txn_commit(txn);
        opened = true;
    }
}

void Core::Storage::close()
{
    if (opened) {
        mdb_dbi_close(environment, base);
        mdb_env_close(environment);
        opened = false;
    }
}

void Core::Storage::addRecord(const QString& key, const QString& value)
{
    if (!opened) {
        throw Archive::Closed("addElement", name.toStdString());
    }
    const std::string& id = key.toStdString();
    const std::string& val = value.toStdString();
    
    MDB_val lmdbKey, lmdbData;
    lmdbKey.mv_size = id.size();
    lmdbKey.mv_data = (char*)id.c_str();
    lmdbData.mv_size = val.size();
    lmdbData.mv_data = (char*)val.c_str();
    MDB_txn *txn;
    mdb_txn_begin(environment, NULL, 0, &txn);
    int rc;
    rc = mdb_put(txn, base, &lmdbKey, &lmdbData, MDB_NOOVERWRITE);
    if (rc != 0) {
        mdb_txn_abort(txn);
        if (rc == MDB_KEYEXIST) {
            throw Archive::Exist(name.toStdString(), id);
        } else {
            throw Archive::Unknown(name.toStdString(), mdb_strerror(rc));
        }
    } else {
        mdb_txn_commit(txn);
    }
}

QString Core::Storage::getRecord(const QString& key) const
{
    if (!opened) {
        throw Archive::Closed("addElement", name.toStdString());
    }
    const std::string& id = key.toStdString();
    
    MDB_val lmdbKey, lmdbData;
    lmdbKey.mv_size = id.size();
    lmdbKey.mv_data = (char*)id.c_str();
    
    MDB_txn *txn;
    int rc;
    mdb_txn_begin(environment, NULL, MDB_RDONLY, &txn);
    rc = mdb_get(txn, base, &lmdbKey, &lmdbData);
    if (rc) {
        mdb_txn_abort(txn);
        if (rc == MDB_NOTFOUND) {
            throw Archive::NotFound(id, name.toStdString());
        } else {
            throw Archive::Unknown(name.toStdString(), mdb_strerror(rc));
        }
    } else {
        std::string sId((char*)lmdbData.mv_data, lmdbData.mv_size);
        QString value(sId.c_str());
        mdb_txn_abort(txn);
        return value;
    }
}

void Core::Storage::removeRecord(const QString& key)
{
    if (!opened) {
        throw Archive::Closed("addElement", name.toStdString());
    }
    const std::string& id = key.toStdString();
    
    MDB_val lmdbKey;
    lmdbKey.mv_size = id.size();
    lmdbKey.mv_data = (char*)id.c_str();
    
    MDB_txn *txn;
    int rc;
    mdb_txn_begin(environment, NULL, 0, &txn);
    rc = mdb_del(txn, base, &lmdbKey, NULL);
    if (rc) {
        mdb_txn_abort(txn);
        if (rc == MDB_NOTFOUND) {
            throw Archive::NotFound(id, name.toStdString());
        } else {
            throw Archive::Unknown(name.toStdString(), mdb_strerror(rc));
        }
    } else {
        mdb_txn_commit(txn);
    }
}
