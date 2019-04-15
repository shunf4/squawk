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
    environment(lmdb::env::create()),
    dbi(0),
    order(0)
{
    
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
        
        environment.set_mapsize(1UL * 1024UL * 1024UL * 1024UL);
        environment.set_max_dbs(2);
        environment.open(path.toStdString().c_str(), 0, 0664);
        
        lmdb::txn wTrans = lmdb::txn::begin(environment);
        dbi = lmdb::dbi::open(wTrans, "main", MDB_CREATE);
        order = lmdb::dbi::open(wTrans, "order", MDB_CREATE | MDB_INTEGERKEY);
        wTrans.commit();
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
    
    lmdb::val key((quint8*)id.c_str(), 36);
    lmdb::val value(ba.data(), ba.size());
    lmdb::txn wTrans = lmdb::txn::begin(environment);
    bool result = dbi.put(wTrans, key, value);
    if (result) {
        lmdb::val oKey((quint8*) &stamp, 8);
        bool oResult = order.put(wTrans, oKey, key);
        if (!oResult) {
            qDebug() << "An element couldn't be inserted into the index";
        }
    } else {
        qDebug() << "An element couldn't been added to the archive, skipping";
    }
    wTrans.commit();
}

void Core::Archive::clear()
{
    if (!opened) {
        throw Closed("clear", jid.toStdString());
    }
    
    lmdb::txn transaction = lmdb::txn::begin(environment);
    dbi.drop(transaction);
    order.drop(transaction);
    transaction.commit();
}

Shared::Message Core::Archive::getElement(const QString& id)
{
    if (!opened) {
        throw Closed("getElement", jid.toStdString());
    }
    
    qDebug() << "getting an element with id " << id.toStdString().c_str();
    lmdb::txn rtxn = lmdb::txn::begin(environment);
    lmdb::val key((quint8*)id.toStdString().c_str(), 36);
    lmdb::val value;
    
    
    int rc = 0;
    char *c_key=(char *)id.toStdString().c_str();
    MDB_val d_key, data;
    data.mv_data = nullptr;
    data.mv_size = 0;
    MDB_txn *txn = nullptr;
    rc = mdb_txn_begin(environment, NULL, MDB_RDONLY, &txn);
    d_key.mv_size =  key.size();
    d_key.mv_data = c_key;
    rc= mdb_get(txn,dbi ,&d_key, &data); 
    if (rc) {
        qDebug() <<"Data Can't be Found, Error: "<<mdb_strerror(rc);
    }
    else if(rc==0)
        qDebug() << "Data Found.\n";
    
    
    
    
    
    
    
    lmdb::cursor cursor = lmdb::cursor::open(rtxn, dbi);
    lmdb::val tKey;
    lmdb::val tValue;
    
    while(cursor.get(tKey, tValue, MDB_NEXT)) {
        std::string sId(tKey.data(), tKey.size());
        qDebug() << "comparing " << id.toStdString().c_str() << " with " << sId.c_str();
        if (sId == id.toStdString()) {
            qDebug() << "EQUALS";
        } else {
            qDebug() << "NOT";
        }
        qDebug() << "sizes: " << key.size() << " : " << tKey.size();
    }
    
    cursor.close();
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    if (dbi.get(rtxn, key, value)) {
        QByteArray ba(value.data(), value.size());
        QDataStream ds(&ba, QIODevice::ReadOnly);
        
        Shared::Message msg;
        msg.deserialize(ds);
        rtxn.abort();
        
        return msg;
    } else {
        rtxn.abort();
        throw NotFound(id.toStdString(), jid.toStdString());
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
    lmdb::txn rtxn = lmdb::txn::begin(environment, nullptr, MDB_RDONLY);
    lmdb::cursor cursor = lmdb::cursor::open(rtxn, order);
    lmdb::val key;
    lmdb::val value;
    
    bool result = cursor.get(key, value, MDB_LAST);
    if (result) {
        std::string sId(value.data(), 36);
        cursor.close();
        rtxn.abort();
        qDebug() << "newest id is " << sId.c_str();
        return sId.c_str();
    } else {
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
    lmdb::txn rtxn = lmdb::txn::begin(environment, nullptr, MDB_RDONLY);
    lmdb::cursor cursor = lmdb::cursor::open(rtxn, order);
    lmdb::val key;
    lmdb::val value;
    
    bool result = cursor.get(key, value, MDB_FIRST);
    if (result) {
        std::string sId;
        sId.assign(value.data(), value.size());
        cursor.close();
        rtxn.abort();
        qDebug() << "Oldest id is " << sId.c_str();
        return sId.c_str();
    } else {
        throw new Empty(jid.toStdString());
    }
}

long unsigned int Core::Archive::size() const
{
    lmdb::txn rtxn = lmdb::txn::begin(environment, nullptr, MDB_RDONLY);
    long unsigned int s = order.size(rtxn);
    rtxn.abort();
    return s;
}
