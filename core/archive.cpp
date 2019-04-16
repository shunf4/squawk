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
#include <QDataStream>
#include <QDir>

Core::Archive::Archive(const QString& p_jid, QObject* parent):
    QObject(parent),
    jid(p_jid),
    opened(false),
    environment(),
    main(),
    order()
{
    mdb_env_create(&environment);
}

Core::Archive::~Archive()
{
}

void Core::Archive::open(const QString& account)
{
    if (!opened) {
        QString path(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
        path += "/" + account + "/" + jid;
        QDir cache(path);
        
        if (!cache.exists()) {
            bool res = cache.mkpath(path);
            if (!res) {
                throw Directory(path.toStdString());
            }
        }
        
        mdb_env_set_maxdbs(environment, 2);
        mdb_env_open(environment, path.toStdString().c_str(), 0, 0664);
        
        int rc;
        MDB_txn *txn;
        rc = mdb_txn_begin(environment, NULL, 0, &txn);
        if (rc) {
            qDebug() << "opening transaction error " << mdb_strerror(rc);
        }
        rc = mdb_dbi_open(txn, "main", MDB_CREATE, &main);
        if (rc) {
            qDebug() << "main opening error " << mdb_strerror(rc);
        }
        rc = mdb_dbi_open(txn, "order", MDB_CREATE | MDB_INTEGERKEY, &order);
        if (rc) {
            qDebug() << "order opening error " << mdb_strerror(rc);
        }
        rc = mdb_txn_commit(txn);
        if (rc) {
            qDebug() << "opening commit transaction error " << mdb_strerror(rc);
        }
        opened = true;
    }
}

void Core::Archive::addElement(const Shared::Message& message)
{
    if (!opened) {
        throw Closed("addElement", jid.toStdString());
    }
    QByteArray ba;
    QDataStream ds(&ba, QIODevice::WriteOnly);
    message.serialize(ds);
    quint64 stamp = message.getTime().toMSecsSinceEpoch();
    const std::string& id = message.getId().toStdString();
    
    qDebug() << "inserting element with id " << id.c_str();
    qDebug() << "data size is " << ba.size();
    
    MDB_val lmdbKey, lmdbData;
    lmdbKey.mv_size = id.size();
    lmdbKey.mv_data = (uint8_t*)id.c_str();
    lmdbData.mv_size = ba.size();
    lmdbData.mv_data = (uint8_t*)ba.data();
    MDB_txn *txn;
    mdb_txn_begin(environment, NULL, 0, &txn);
    int rc;
    rc = mdb_put(txn, main, &lmdbKey, &lmdbData, 0);
    if (rc == 0) {
        MDB_val orderKey;
        orderKey.mv_size = 8;
        orderKey.mv_data = (uint8_t*) &stamp;
        
        rc = mdb_put(txn, order, &orderKey, &lmdbKey, 0);
        if (rc) {
            qDebug() << "An element couldn't be inserted into the index" << mdb_strerror(rc);
            mdb_txn_abort(txn);
        } else {
            rc = mdb_txn_commit(txn);
            if (rc) {
                qDebug() << "A transaction error: " << mdb_strerror(rc);
            }
        }
    } else {
        qDebug() << "An element couldn't been added to the archive, skipping" << mdb_strerror(rc);
        mdb_txn_abort(txn);
    }
}

void Core::Archive::clear()
{
    if (!opened) {
        throw Closed("clear", jid.toStdString());
    }
    
    MDB_txn *txn;
    mdb_txn_begin(environment, NULL, 0, &txn);
    mdb_drop(txn, main, 0);
    mdb_drop(txn, order, 0);
    mdb_txn_commit(txn);
}

Shared::Message Core::Archive::getElement(const QString& id)
{
    if (!opened) {
        throw Closed("getElement", jid.toStdString());
    }
    
    qDebug() << "getting an element with id " << id.toStdString().c_str();
    
    MDB_val lmdbKey, lmdbData;
    lmdbKey.mv_size = id.toStdString().size();
    lmdbKey.mv_data = (uint8_t*)id.toStdString().c_str();
    
    MDB_txn *txn;
    int rc;
    rc = mdb_txn_begin(environment, NULL, MDB_RDONLY, &txn);
    if (rc) {
        qDebug() <<"Get error: " << mdb_strerror(rc);
        mdb_txn_abort(txn);
        throw NotFound(id.toStdString(), jid.toStdString());
    }
    rc = mdb_get(txn, main, &lmdbKey, &lmdbData);
    if (rc) {
        qDebug() <<"Get error: " << mdb_strerror(rc);
        mdb_txn_abort(txn);
        throw NotFound(id.toStdString(), jid.toStdString());
    } else {
        QByteArray ba((char*)lmdbData.mv_data, lmdbData.mv_size);
        QDataStream ds(&ba, QIODevice::ReadOnly);
        
        Shared::Message msg;
        msg.deserialize(ds);
        mdb_txn_abort(txn);
        return msg;
    }
}

Shared::Message Core::Archive::newest()
{
    QString id = newestId();
    return getElement(id);
}

QString Core::Archive::newestId()
{
    if (!opened) {
        throw Closed("newestId", jid.toStdString());
    }
    MDB_txn *txn;
    int rc;
    rc = mdb_txn_begin(environment, NULL, MDB_RDONLY, &txn);
    MDB_cursor* cursor;
    rc = mdb_cursor_open(txn, order, &cursor);
    MDB_val lmdbKey, lmdbData;
    
    rc = mdb_cursor_get(cursor, &lmdbKey, &lmdbData, MDB_LAST);
    if (rc) {
        std::string sId((char*)lmdbData.mv_data, lmdbData.mv_size);
        mdb_cursor_close(cursor);
        mdb_txn_abort(txn);
        qDebug() << "newest id is " << sId.c_str();
        return sId.c_str();
    } else {
        qDebug() << "Error geting newestId " << mdb_strerror(rc);
        mdb_cursor_close(cursor);
        mdb_txn_abort(txn);
        throw new Empty(jid.toStdString());
    }
}

Shared::Message Core::Archive::oldest()
{
    return getElement(oldestId());
}

QString Core::Archive::oldestId()
{
    if (!opened) {
        throw Closed("oldestId", jid.toStdString());
    }
    MDB_txn *txn;
    int rc;
    rc = mdb_txn_begin(environment, NULL, MDB_RDONLY, &txn);
    MDB_cursor* cursor;
    rc = mdb_cursor_open(txn, order, &cursor);
    MDB_val lmdbKey, lmdbData;
    
    rc = mdb_cursor_get(cursor, &lmdbKey, &lmdbData, MDB_FIRST);
    if (rc) {
        std::string sId((char*)lmdbData.mv_data, lmdbData.mv_size);
        mdb_cursor_close(cursor);
        mdb_txn_abort(txn);
        qDebug() << "oldest id is " << sId.c_str();
        return sId.c_str();
    } else {
        qDebug() << "Error geting oldestId " << mdb_strerror(rc);
        mdb_cursor_close(cursor);
        mdb_txn_abort(txn);
        throw new Empty(jid.toStdString());
    }
}

long unsigned int Core::Archive::size() const
{
    MDB_txn *txn;
    int rc;
    rc = mdb_txn_begin(environment, NULL, MDB_RDONLY, &txn);
    MDB_stat stat;
    mdb_stat(txn, order, &stat);
    mdb_txn_abort(txn);
    return stat.ms_entries;
}
