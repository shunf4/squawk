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
    fromTheBeginning(false),
    environment(),
    main(),
    order()
{
}

Core::Archive::~Archive()
{
    close();
}

void Core::Archive::open(const QString& account)
{
    if (!opened) {
        mdb_env_create(&environment);
        QString path(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
        path += "/" + account + "/" + jid;
        QDir cache(path);
        
        if (!cache.exists()) {
            bool res = cache.mkpath(path);
            if (!res) {
                throw Directory(path.toStdString());
            }
        }
        
        mdb_env_set_maxdbs(environment, 3);
        mdb_env_open(environment, path.toStdString().c_str(), 0, 0664);
        
        MDB_txn *txn;
        mdb_txn_begin(environment, NULL, 0, &txn);
        mdb_dbi_open(txn, "main", MDB_CREATE, &main);
        mdb_dbi_open(txn, "order", MDB_CREATE | MDB_INTEGERKEY, &order);
        mdb_dbi_open(txn, "order", MDB_CREATE, &stats);
        mdb_txn_commit(txn);
        fromTheBeginning = _isFromTheBeginning();
        opened = true;
    }
}

void Core::Archive::close()
{
    if (opened) {
        mdb_dbi_close(environment, stats);
        mdb_dbi_close(environment, order);
        mdb_dbi_close(environment, main);
        mdb_env_close(environment);
        opened = false;
    }
}

bool Core::Archive::addElement(const Shared::Message& message)
{
    if (!opened) {
        throw Closed("addElement", jid.toStdString());
    }
    QByteArray ba;
    QDataStream ds(&ba, QIODevice::WriteOnly);
    message.serialize(ds);
    quint64 stamp = message.getTime().toMSecsSinceEpoch();
    const std::string& id = message.getId().toStdString();
    
    MDB_val lmdbKey, lmdbData;
    lmdbKey.mv_size = id.size();
    lmdbKey.mv_data = (char*)id.c_str();
    lmdbData.mv_size = ba.size();
    lmdbData.mv_data = (uint8_t*)ba.data();
    MDB_txn *txn;
    mdb_txn_begin(environment, NULL, 0, &txn);
    int rc;
    rc = mdb_put(txn, main, &lmdbKey, &lmdbData, MDB_NOOVERWRITE);
    if (rc == 0) {
        MDB_val orderKey;
        orderKey.mv_size = 8;
        orderKey.mv_data = (uint8_t*) &stamp;
        
        rc = mdb_put(txn, order, &orderKey, &lmdbKey, 0);
        if (rc) {
            qDebug() << "An element couldn't be inserted into the index" << mdb_strerror(rc);
            mdb_txn_abort(txn);
            return false;
        } else {
            rc = mdb_txn_commit(txn);
            if (rc) {
                qDebug() << "A transaction error: " << mdb_strerror(rc);
                return false;
            }
            return true;
        }
    } else {
        qDebug() << "An element couldn't been added to the archive, skipping" << mdb_strerror(rc);
        mdb_txn_abort(txn);
        return false;
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
    
    std::string strKey = id.toStdString();
    
    MDB_val lmdbKey, lmdbData;
    lmdbKey.mv_size = strKey.size();
    lmdbKey.mv_data = (char*)strKey.c_str();
    
    MDB_txn *txn;
    int rc;
    mdb_txn_begin(environment, NULL, MDB_RDONLY, &txn);
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
        qDebug() << "Error geting newestId " << mdb_strerror(rc);
        mdb_cursor_close(cursor);
        mdb_txn_abort(txn);
        throw new Empty(jid.toStdString());
    } else {
        std::string sId((char*)lmdbData.mv_data, lmdbData.mv_size);
        mdb_cursor_close(cursor);
        mdb_txn_abort(txn);
        return sId.c_str();
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
        qDebug() << "Error geting oldestId " << mdb_strerror(rc);
        mdb_cursor_close(cursor);
        mdb_txn_abort(txn);
        throw new Empty(jid.toStdString());
    } else {
        std::string sId((char*)lmdbData.mv_data, lmdbData.mv_size);
        mdb_cursor_close(cursor);
        mdb_txn_abort(txn);
        return sId.c_str();
    }
}

long unsigned int Core::Archive::size() const
{
    if (!opened) {
        throw Closed("size", jid.toStdString());
    }
    MDB_txn *txn;
    int rc;
    rc = mdb_txn_begin(environment, NULL, MDB_RDONLY, &txn);
    MDB_stat stat;
    mdb_stat(txn, order, &stat);
    mdb_txn_abort(txn);
    return stat.ms_entries;
}

std::list<Shared::Message> Core::Archive::getBefore(int count, const QString& id)
{
    if (!opened) {
        throw Closed("getBefore", jid.toStdString());
    }
    std::list<Shared::Message> res;
    MDB_cursor* cursor;
    MDB_txn *txn;
    MDB_val lmdbKey, lmdbData;
    int rc;
    rc = mdb_txn_begin(environment, NULL, MDB_RDONLY, &txn);
    if (id == "") {
        rc = mdb_cursor_open(txn, order, &cursor);
        rc = mdb_cursor_get(cursor, &lmdbKey, &lmdbData, MDB_LAST);
        if (rc) {
            qDebug() << "Error geting before " << mdb_strerror(rc);
            mdb_cursor_close(cursor);
            mdb_txn_abort(txn);
            throw new Empty(jid.toStdString());
        } else {
            std::string sId((char*)lmdbData.mv_data, lmdbData.mv_size);
        }
    } else {
        lmdbKey.mv_size = id.size();
        lmdbKey.mv_data = (char*)id.toStdString().c_str();
        rc = mdb_get(txn, main, &lmdbKey, &lmdbData);
        if (rc) {
            qDebug() <<"Error getting before: " << mdb_strerror(rc);
            mdb_txn_abort(txn);
            throw NotFound(id.toStdString(), jid.toStdString());
        } else {
            QByteArray ba((char*)lmdbData.mv_data, lmdbData.mv_size);
            QDataStream ds(&ba, QIODevice::ReadOnly);
            
            Shared::Message msg;
            msg.deserialize(ds);
            quint64 stamp = msg.getTime().toMSecsSinceEpoch();
            lmdbKey.mv_data = (quint8*)&stamp;
            lmdbKey.mv_size = 8;
            
            rc = mdb_cursor_open(txn, order, &cursor);
            rc = mdb_cursor_get(cursor, &lmdbKey, &lmdbData, MDB_SET);
            
            if (rc) {
                qDebug() << "Error getting before " << mdb_strerror(rc);
                mdb_cursor_close(cursor);
                mdb_txn_abort(txn);
                throw new NotFound(id.toStdString(), jid.toStdString());
            } else {
                rc = mdb_cursor_get(cursor, &lmdbKey, &lmdbData, MDB_PREV);
                if (rc) {
                    qDebug() << "Error getting before " << mdb_strerror(rc);
                    mdb_cursor_close(cursor);
                    mdb_txn_abort(txn);
                    throw new NotFound(id.toStdString(), jid.toStdString());
                }
            }
        }
    }
    
    do {
        QByteArray ba((char*)lmdbData.mv_data, lmdbData.mv_size);
        QDataStream ds(&ba, QIODevice::ReadOnly);
        
        res.emplace_back();
        Shared::Message& msg = res.back();
        msg.deserialize(ds);
        
        --count;
        
    } while (count > 0 && mdb_cursor_get(cursor, &lmdbKey, &lmdbData, MDB_PREV) == 0);
    
    mdb_cursor_close(cursor);
    mdb_txn_abort(txn);
    return res;
}

bool Core::Archive::_isFromTheBeginning()
{
    std::string strKey = "beginning";
    
    MDB_val lmdbKey, lmdbData;
    lmdbKey.mv_size = strKey.size();
    lmdbKey.mv_data = (char*)strKey.c_str();
    
    MDB_txn *txn;
    int rc;
    mdb_txn_begin(environment, NULL, MDB_RDONLY, &txn);
    rc = mdb_get(txn, stats, &lmdbKey, &lmdbData);
    if (rc == MDB_NOTFOUND) {
        mdb_txn_abort(txn);
        return false;
    } else if (rc) {
        qDebug() <<"isFromTheBeginning error: " << mdb_strerror(rc);
        mdb_txn_abort(txn);
        throw NotFound(strKey, jid.toStdString());
    } else {
        uint8_t value = *(uint8_t*)(lmdbData.mv_data);
        bool is;
        if (value == 144) {
            is = false;
        } else if (value == 72) {
            is = true;
        } else {
            qDebug() <<"isFromTheBeginning error: stored value doesn't match any magic number, the answer is most probably wrong";
        }
        return is;
    }
}

bool Core::Archive::isFromTheBeginning()
{
    if (!opened) {
        throw Closed("isFromTheBeginning", jid.toStdString());
    }
    return fromTheBeginning;
}

bool Core::Archive::setFromTheBeginning(bool is)
{
    if (!opened) {
        throw Closed("setFromTheBeginning", jid.toStdString());
    }
    if (fromTheBeginning != is) {
        fromTheBeginning = is;
        const std::string& id = "beginning";
        uint8_t value = 144;
        if (is) {
            value = 72;
        }
        
        MDB_val lmdbKey, lmdbData;
        lmdbKey.mv_size = id.size();
        lmdbKey.mv_data = (char*)id.c_str();
        lmdbData.mv_size = sizeof value;
        lmdbData.mv_data = &value;
        MDB_txn *txn;
        mdb_txn_begin(environment, NULL, 0, &txn);
        int rc;
        rc = mdb_put(txn, stats, &lmdbKey, &lmdbData, 0);
        if (rc != 0) {
            qDebug() << "Couldn't store beginning key into stat database:" << mdb_strerror(rc);
            mdb_txn_abort(txn);
        }
    }
}
