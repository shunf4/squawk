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
    order(),
    stats(),
    avatars()
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
        
        mdb_env_set_maxdbs(environment, 5);
        mdb_env_set_mapsize(environment, 512UL * 1024UL * 1024UL);
        mdb_env_open(environment, path.toStdString().c_str(), 0, 0664);
        
        MDB_txn *txn;
        mdb_txn_begin(environment, NULL, 0, &txn);
        mdb_dbi_open(txn, "main", MDB_CREATE, &main);
        mdb_dbi_open(txn, "order", MDB_CREATE | MDB_INTEGERKEY, &order);
        mdb_dbi_open(txn, "stats", MDB_CREATE, &stats);
        mdb_dbi_open(txn, "avatars", MDB_CREATE, &avatars);
        mdb_dbi_open(txn, "sid", MDB_CREATE, &sid);
        mdb_txn_commit(txn);
        
        mdb_txn_begin(environment, NULL, MDB_RDONLY, &txn);
        try {
            fromTheBeginning = getStatBoolValue("beginning", txn);
        } catch (const NotFound& e) {
            fromTheBeginning = false;
        }
        
        std::string sJid = jid.toStdString();
        AvatarInfo info;
        bool hasAvatar = readAvatarInfo(info, sJid, txn);
        mdb_txn_abort(txn);
        
        if (hasAvatar) {
            QFile ava(path + "/" + sJid.c_str() + "." + info.type);
            if (!ava.exists()) {
                bool success = dropAvatar(sJid);
                if (!success) {
                    qDebug() << "error opening archive" << jid << "for account" << account 
                    << ". There is supposed to be avatar but the file doesn't exist, couldn't even drop it, it surely will lead to an error";
                }
            }
        }
        
        opened = true;
    }
}

void Core::Archive::close()
{
    if (opened) {
        mdb_dbi_close(environment, sid);
        mdb_dbi_close(environment, avatars);
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
            if (message.getStanzaId().size() > 0) {
                const std::string& szid = message.getStanzaId().toStdString();
                
                lmdbKey.mv_size = szid.size();
                lmdbKey.mv_data = (char*)szid.c_str();
                lmdbData.mv_size = id.size();
                lmdbData.mv_data = (uint8_t*)id.data();
                rc = mdb_put(txn, sid, &lmdbKey, &lmdbData, MDB_NOOVERWRITE);
                
                if (rc) {
                    qDebug() << "An element stanzaId to id pair couldn't be inserted into the archive" << mdb_strerror(rc);
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
                rc = mdb_txn_commit(txn);
                if (rc) {
                    qDebug() << "A transaction error: " << mdb_strerror(rc);
                    return false;
                }
                return true;
            }
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
    mdb_drop(txn, stats, 0);
    mdb_drop(txn, avatars, 0);
    mdb_drop(txn, sid, 0);
    mdb_txn_commit(txn);
}

Shared::Message Core::Archive::getElement(const QString& id) const
{
    if (!opened) {
        throw Closed("getElement", jid.toStdString());
    }
    
    MDB_txn *txn;
    mdb_txn_begin(environment, NULL, MDB_RDONLY, &txn);
    
    try {
        Shared::Message msg = getMessage(id.toStdString(), txn);
        mdb_txn_abort(txn);
        return msg;
    } catch (...) {
        mdb_txn_abort(txn);
        throw;
    }
}

bool Core::Archive::hasElement(const QString& id) const
{
    if (!opened) {
        throw Closed("hasElement", jid.toStdString());
    }
    
    MDB_txn *txn;
    mdb_txn_begin(environment, NULL, MDB_RDONLY, &txn);
    
    bool has;
    MDB_val lmdbKey, lmdbData;
    lmdbKey.mv_size = id.size();
    lmdbKey.mv_data = (char*)id.toStdString().c_str();
    int rc = mdb_get(txn, main, &lmdbKey, &lmdbData);
    has = rc == 0;
    mdb_txn_abort(txn);
    
    return has;
}

Shared::Message Core::Archive::getMessage(const std::string& id, MDB_txn* txn) const
{
    MDB_val lmdbKey, lmdbData;
    lmdbKey.mv_size = id.size();
    lmdbKey.mv_data = (char*)id.c_str();
    int rc = mdb_get(txn, main, &lmdbKey, &lmdbData);
    
    if (rc == 0) {
        QByteArray ba((char*)lmdbData.mv_data, lmdbData.mv_size);
        QDataStream ds(&ba, QIODevice::ReadOnly);
        
        Shared::Message msg;
        msg.deserialize(ds);
        
        return msg;
    } else if (rc == MDB_NOTFOUND) {
        throw NotFound(id, jid.toStdString());
    } else {
        throw Unknown(jid.toStdString(), mdb_strerror(rc));
    }
}

void Core::Archive::changeMessage(const QString& id, const QMap<QString, QVariant>& data)
{
    if (!opened) {
        throw Closed("setMessageState", jid.toStdString());
    }
    
    MDB_txn *txn;
    mdb_txn_begin(environment, NULL, 0, &txn);
    
    std::string strId(id.toStdString());
    try {
        Shared::Message msg = getMessage(strId, txn);
        bool hadStanzaId = msg.getStanzaId().size() > 0;
        QDateTime oTime = msg.getTime();
        bool idChange = msg.change(data);
        
        MDB_val lmdbKey, lmdbData;
        QByteArray ba;
        QDataStream ds(&ba, QIODevice::WriteOnly);
        msg.serialize(ds);
        
        lmdbKey.mv_size = strId.size();
        lmdbKey.mv_data = (char*)strId.c_str();
        int rc;
        if (idChange) {
            rc = mdb_del(txn, main, &lmdbKey, &lmdbData);
            if (rc == 0) {
                strId = msg.getId().toStdString();
                lmdbKey.mv_size = strId.size();
                lmdbKey.mv_data = (char*)strId.c_str();
                
                
                quint64 stamp = oTime.toMSecsSinceEpoch();
                lmdbData.mv_data = (quint8*)&stamp;
                lmdbData.mv_size = 8;
                rc = mdb_put(txn, order, &lmdbData, &lmdbKey, 0);
                if (rc != 0) {
                    throw Unknown(jid.toStdString(), mdb_strerror(rc));
                }
            } else {
                throw Unknown(jid.toStdString(), mdb_strerror(rc));
            }
        }
        
        if (msg.getStanzaId().size() > 0 && (idChange || !hadStanzaId)) {
            const std::string& szid = msg.getStanzaId().toStdString();
            
            lmdbData.mv_size = szid.size();
            lmdbData.mv_data = (char*)szid.c_str();
            rc = mdb_put(txn, sid, &lmdbData, &lmdbKey, 0);
            
            if (rc != 0) {
                throw Unknown(jid.toStdString(), mdb_strerror(rc));
            }
        };
        
        lmdbData.mv_size = ba.size();
        lmdbData.mv_data = (uint8_t*)ba.data();
        rc = mdb_put(txn, main, &lmdbKey, &lmdbData, 0);
        if (rc == 0) {
            rc = mdb_txn_commit(txn);
        } else {
            throw Unknown(jid.toStdString(), mdb_strerror(rc));
        }
        
    } catch (...) {
        mdb_txn_abort(txn);
        throw;
    }
}

Shared::Message Core::Archive::newest()
{
    return edge(true);
}

QString Core::Archive::newestId()
{
    if (!opened) {
        throw Closed("newestId", jid.toStdString());
    }
    Shared::Message msg = newest();
    return msg.getId();
}

QString Core::Archive::oldestId()
{
    if (!opened) {
        throw Closed("oldestId", jid.toStdString());
    }
    Shared::Message msg = oldest();
    return msg.getId();
}

Shared::Message Core::Archive::oldest() 
{
    return edge(false);
}

Shared::Message Core::Archive::edge(bool end)
{
    QString name;
    MDB_cursor_op begin;
    MDB_cursor_op iteration;
    if (end) {
        name = "newest";
        begin = MDB_LAST;
        iteration = MDB_PREV;
    } else {
        name = "oldest";
        begin = MDB_FIRST;
        iteration = MDB_NEXT;
    }
    
    
    if (!opened) {
        throw Closed(name.toStdString(), jid.toStdString());
    }
    
    MDB_txn *txn;
    MDB_cursor* cursor;
    MDB_val lmdbKey, lmdbData;
    int rc;
    rc = mdb_txn_begin(environment, NULL, MDB_RDONLY, &txn);
    rc = mdb_cursor_open(txn, order, &cursor);
    rc = mdb_cursor_get(cursor, &lmdbKey, &lmdbData, begin);
    
    Shared::Message msg = getStoredMessage(txn, cursor, iteration, &lmdbKey, &lmdbData, rc);
    
    mdb_cursor_close(cursor);
    mdb_txn_abort(txn);
    
    if (rc) {
        qDebug() << "Error geting" << name << "message" << mdb_strerror(rc);
        throw Empty(jid.toStdString());
    } else {
        return msg;
    }
}

Shared::Message Core::Archive::getStoredMessage(MDB_txn *txn, MDB_cursor* cursor, MDB_cursor_op op, MDB_val* key, MDB_val* value, int& rc)
{
    Shared::Message msg;
    std::string sId;
    while (true) {
        if (rc) {
            break;
        }
        sId = std::string((char*)value->mv_data, value->mv_size);
        
        try {
            msg = getMessage(sId, txn);
            if (msg.serverStored()) {
                break;
            } else {
                rc = mdb_cursor_get(cursor, key, value, op);
            }
        } catch (...) {
            break;
        }
    }
    
    return msg;
}

unsigned int Core::Archive::addElements(const std::list<Shared::Message>& messages)
{
    if (!opened) {
        throw Closed("addElements", jid.toStdString());
    }
    
    int success = 0;
    int rc = 0;
    MDB_val lmdbKey, lmdbData;
    MDB_txn *txn;
    mdb_txn_begin(environment, NULL, 0, &txn);
    std::list<Shared::Message>::const_iterator itr = messages.begin();
    while (rc == 0 && itr != messages.end()) {
        const Shared::Message& message = *itr;
        
        QByteArray ba;
        QDataStream ds(&ba, QIODevice::WriteOnly);
        message.serialize(ds);
        quint64 stamp = message.getTime().toMSecsSinceEpoch();
        const std::string& id = message.getId().toStdString();
        
        lmdbKey.mv_size = id.size();
        lmdbKey.mv_data = (char*)id.c_str();
        lmdbData.mv_size = ba.size();
        lmdbData.mv_data = (uint8_t*)ba.data();
        
        rc = mdb_put(txn, main, &lmdbKey, &lmdbData, MDB_NOOVERWRITE);
        if (rc == 0) {
            MDB_val orderKey;
            orderKey.mv_size = 8;
            orderKey.mv_data = (uint8_t*) &stamp;
            
            rc = mdb_put(txn, order, &orderKey, &lmdbKey, 0);
            if (rc) {
                qDebug() << "An element couldn't be inserted into the index, aborting the transaction" << mdb_strerror(rc);
            } else {
                if (message.getStanzaId().size() > 0) {
                    const std::string& szid = message.getStanzaId().toStdString();
                    
                    lmdbKey.mv_size = szid.size();
                    lmdbKey.mv_data = (char*)szid.c_str();
                    lmdbData.mv_size = id.size();
                    lmdbData.mv_data = (uint8_t*)id.data();
                    rc = mdb_put(txn, sid, &lmdbKey, &lmdbData, MDB_NOOVERWRITE);
                    
                    if (rc) {
                        qDebug() << "During bulk add an element stanzaId to id pair couldn't be inserted into the archive, continuing without stanzaId" << mdb_strerror(rc);
                    }
                    
                }
                success++;
            }
        } else {
            if (rc == MDB_KEYEXIST) {
                rc = 0;
            } else {
                qDebug() << "An element couldn't been added to the archive, aborting the transaction" << mdb_strerror(rc);
            }
        }
        itr++;
    }
    
    if (rc != 0) {
        mdb_txn_abort(txn);
        success = 0;
    } else {
        mdb_txn_commit(txn);
    }
    
    return success;
}

long unsigned int Core::Archive::size() const
{
    if (!opened) {
        throw Closed("size", jid.toStdString());
    }
    MDB_txn *txn;
    mdb_txn_begin(environment, NULL, MDB_RDONLY, &txn);
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
    rc = mdb_cursor_open(txn, order, &cursor);
    if (id == "") {
        rc = mdb_cursor_get(cursor, &lmdbKey, &lmdbData, MDB_LAST);
        if (rc) {
            qDebug() << "Error getting before" << mdb_strerror(rc) << ", id:" << id;
            mdb_cursor_close(cursor);
            mdb_txn_abort(txn);
            
            throw Empty(jid.toStdString());
        }
    } else {
        std::string stdId(id.toStdString());
        try {
            Shared::Message msg = getMessage(stdId, txn);
            quint64 stamp = msg.getTime().toMSecsSinceEpoch();
            lmdbKey.mv_data = (quint8*)&stamp;
            lmdbKey.mv_size = 8;
            
            rc = mdb_cursor_get(cursor, &lmdbKey, &lmdbData, MDB_SET);
            
            if (rc) {
                qDebug() << "Error getting before: couldn't set " << mdb_strerror(rc);
                throw NotFound(stdId, jid.toStdString());
            } else {
                rc = mdb_cursor_get(cursor, &lmdbKey, &lmdbData, MDB_PREV);
                if (rc) {
                    qDebug() << "Error getting before, couldn't prev " << mdb_strerror(rc);
                    throw NotFound(stdId, jid.toStdString());
                }
            }
            
        } catch (...) {
            mdb_cursor_close(cursor);
            mdb_txn_abort(txn);
            throw;
        }
    }
    
    do {
        MDB_val dKey, dData;
        dKey.mv_size = lmdbData.mv_size;
        dKey.mv_data = lmdbData.mv_data;
        rc = mdb_get(txn, main, &dKey, &dData);
        if (rc) {
            qDebug() <<"Get error: " << mdb_strerror(rc);
            std::string sId((char*)lmdbData.mv_data, lmdbData.mv_size);
            mdb_cursor_close(cursor);
            mdb_txn_abort(txn);
            throw NotFound(sId, jid.toStdString());
        } else {
            QByteArray ba((char*)dData.mv_data, dData.mv_size);
            QDataStream ds(&ba, QIODevice::ReadOnly);
            
            res.emplace_back();
            Shared::Message& msg = res.back();
            msg.deserialize(ds);
        }
        
        --count;
        
    } while (count > 0 && mdb_cursor_get(cursor, &lmdbKey, &lmdbData, MDB_PREV) == 0);
    
    mdb_cursor_close(cursor);
    mdb_txn_abort(txn);
    return res;
}

bool Core::Archive::isFromTheBeginning()
{
    if (!opened) {
        throw Closed("isFromTheBeginning", jid.toStdString());
    }
    return fromTheBeginning;
}

void Core::Archive::setFromTheBeginning(bool is)
{
    if (!opened) {
        throw Closed("setFromTheBeginning", jid.toStdString());
    }
    if (fromTheBeginning != is) {
        fromTheBeginning = is;
        
        MDB_txn *txn;
        mdb_txn_begin(environment, NULL, 0, &txn);
        bool success = setStatValue("beginning", is, txn);
        if (success != 0) {
            mdb_txn_abort(txn);
        } else {
            mdb_txn_commit(txn);
        }
    }
}

QString Core::Archive::idByStanzaId(const QString& stanzaId) const
{
    if (!opened) {
        throw Closed("idByStanzaId", jid.toStdString());
    }
    QString id;
    std::string ssid = stanzaId.toStdString();
    
    MDB_txn *txn;
    MDB_val lmdbKey, lmdbData;
    lmdbKey.mv_size = ssid.size();
    lmdbKey.mv_data = (char*)ssid.c_str();
    mdb_txn_begin(environment, NULL, MDB_RDONLY, &txn);
    int rc = mdb_get(txn, sid, &lmdbKey, &lmdbData);
    if (rc == 0) {
        id = QString::fromStdString(std::string((char*)lmdbData.mv_data, lmdbData.mv_size));
    }
    mdb_txn_abort(txn);
    
    return id;
}

QString Core::Archive::stanzaIdById(const QString& id) const
{
    if (!opened) {
        throw Closed("stanzaIdById", jid.toStdString());
    }
    
    try {
        Shared::Message msg = getElement(id);
        return msg.getStanzaId();
    } catch (const NotFound& e) {
        return QString();
    } catch (const Empty& e) {
        return QString();
    } catch (...) {
        throw;
    }
}

void Core::Archive::printOrder()
{
    qDebug() << "Printing order";
    MDB_txn *txn;
    mdb_txn_begin(environment, NULL, MDB_RDONLY, &txn);
    MDB_cursor* cursor;
    mdb_cursor_open(txn, order, &cursor);
    MDB_val lmdbKey, lmdbData;
    
    mdb_cursor_get(cursor, &lmdbKey, &lmdbData, MDB_FIRST);
    
    do {
        std::string sId((char*)lmdbData.mv_data, lmdbData.mv_size);
        qDebug() << QString(sId.c_str());
    } while (mdb_cursor_get(cursor, &lmdbKey, &lmdbData, MDB_NEXT) == 0);
    
    mdb_cursor_close(cursor);
    mdb_txn_abort(txn);
}

void Core::Archive::printKeys()
{
    MDB_txn *txn;
    mdb_txn_begin(environment, NULL, MDB_RDONLY, &txn);
    MDB_cursor* cursor;
    mdb_cursor_open(txn, main, &cursor);
    MDB_val lmdbKey, lmdbData;
    
    mdb_cursor_get(cursor, &lmdbKey, &lmdbData, MDB_FIRST);
    
    do {
        std::string sId((char*)lmdbKey.mv_data, lmdbKey.mv_size);
        qDebug() << QString(sId.c_str());
    } while (mdb_cursor_get(cursor, &lmdbKey, &lmdbData, MDB_NEXT) == 0);
    
    mdb_cursor_close(cursor);
    mdb_txn_abort(txn);
}

bool Core::Archive::getStatBoolValue(const std::string& id, MDB_txn* txn)
{
    MDB_val lmdbKey, lmdbData;
    lmdbKey.mv_size = id.size();
    lmdbKey.mv_data = (char*)id.c_str();
    
    int rc;
    rc = mdb_get(txn, stats, &lmdbKey, &lmdbData);
    if (rc == MDB_NOTFOUND) {
        throw NotFound(id, jid.toStdString());
    } else if (rc) {
        std::string err(mdb_strerror(rc));
        qDebug() << "error retrieving" << id.c_str() << "from stats db of" << jid << err.c_str();
        throw Unknown(jid.toStdString(), err);
    } else {
        uint8_t value = *(uint8_t*)(lmdbData.mv_data);
        bool is;
        if (value == 144) {
            is = false;
        } else if (value == 72) {
            is = true;
        } else {
            qDebug() << "error retrieving boolean stat" << id.c_str() << ": stored value doesn't match any magic number, the answer is most probably wrong";
            throw NotFound(id, jid.toStdString());
        }
        return is;
    }
}

std::string Core::Archive::getStatStringValue(const std::string& id, MDB_txn* txn)
{
    MDB_val lmdbKey, lmdbData;
    lmdbKey.mv_size = id.size();
    lmdbKey.mv_data = (char*)id.c_str();
    
    int rc;
    rc = mdb_get(txn, stats, &lmdbKey, &lmdbData);
    if (rc == MDB_NOTFOUND) {
        throw NotFound(id, jid.toStdString());
    } else if (rc) {
        std::string err(mdb_strerror(rc));
        qDebug() << "error retrieving" << id.c_str() << "from stats db of" << jid << err.c_str();
        throw Unknown(jid.toStdString(), err);
    } else {
        std::string value((char*)lmdbData.mv_data, lmdbData.mv_size);
        return value;
    }
}

bool Core::Archive::setStatValue(const std::string& id, bool value, MDB_txn* txn)
{
    uint8_t binvalue = 144;
    if (value) {
        binvalue = 72;
    }
    MDB_val lmdbKey, lmdbData;
    lmdbKey.mv_size = id.size();
    lmdbKey.mv_data = (char*)id.c_str();
    lmdbData.mv_size = sizeof binvalue;
    lmdbData.mv_data = &binvalue;
    int rc = mdb_put(txn, stats, &lmdbKey, &lmdbData, 0);
    if (rc != 0) {
        qDebug() << "Couldn't store" << id.c_str() << "key into stat database:" << mdb_strerror(rc);
        return false;
    }
    return true;
}

bool Core::Archive::setStatValue(const std::string& id, const std::string& value, MDB_txn* txn)
{
    MDB_val lmdbKey, lmdbData;
    lmdbKey.mv_size = id.size();
    lmdbKey.mv_data = (char*)id.c_str();
    lmdbData.mv_size = value.size();
    lmdbData.mv_data = (char*)value.c_str();
    int rc = mdb_put(txn, stats, &lmdbKey, &lmdbData, 0);
    if (rc != 0) {
        qDebug() << "Couldn't store" << id.c_str() << "key into stat database:" << mdb_strerror(rc);
        return false;
    }
    return true;
}

bool Core::Archive::dropAvatar(const std::string& resource)
{
    MDB_txn *txn;
    MDB_val lmdbKey;
    mdb_txn_begin(environment, NULL, 0, &txn);
    lmdbKey.mv_size = resource.size();
    lmdbKey.mv_data = (char*)resource.c_str();
    int rc = mdb_del(txn, avatars, &lmdbKey, NULL);
    if (rc != 0) {
        mdb_txn_abort(txn);
        return false;
    } else {
        mdb_txn_commit(txn);
        return true;
    }
}

bool Core::Archive::setAvatar(const QByteArray& data, AvatarInfo& newInfo, bool generated, const QString& resource)
{
    if (!opened) {
        throw Closed("setAvatar", jid.toStdString());
    }
    
    AvatarInfo oldInfo;
    bool hasAvatar = readAvatarInfo(oldInfo, resource);
    std::string res = resource.size() == 0 ? jid.toStdString() : resource.toStdString();
    
    if (data.size() == 0) {
        if (!hasAvatar) {
            return false;
        } else {
            return dropAvatar(res);
        }
    } else {
        const char* cep;
        mdb_env_get_path(environment, &cep);
        QString currentPath(cep);
        bool needToRemoveOld = false;
        QCryptographicHash hash(QCryptographicHash::Sha1);
        hash.addData(data);
        QByteArray newHash(hash.result());
        if (hasAvatar) {
            if (!generated && !oldInfo.autogenerated && oldInfo.hash == newHash) {
                return false;
            }
            QFile oldAvatar(currentPath + "/" + res.c_str() + "." + oldInfo.type);
            if (oldAvatar.exists()) {
                if (oldAvatar.rename(currentPath + "/" + res.c_str() + "." + oldInfo.type + ".bak")) {
                    needToRemoveOld = true;
                } else {
                    qDebug() << "Can't change avatar: couldn't get rid of the old avatar" << oldAvatar.fileName();
                    return false;
                }
            }
        }
        QMimeDatabase db;
        QMimeType type = db.mimeTypeForData(data);
        QString ext = type.preferredSuffix();
        QFile newAvatar(currentPath + "/" + res.c_str() + "." + ext);
        if (newAvatar.open(QFile::WriteOnly)) {
            newAvatar.write(data);
            newAvatar.close();
            
            MDB_txn *txn;
            mdb_txn_begin(environment, NULL, 0, &txn);
            
            MDB_val lmdbKey, lmdbData;
            QByteArray value;
            newInfo.type = ext;
            newInfo.hash = newHash;
            newInfo.autogenerated = generated;
            newInfo.serialize(&value);
            lmdbKey.mv_size = res.size();
            lmdbKey.mv_data = (char*)res.c_str();
            lmdbData.mv_size = value.size();
            lmdbData.mv_data = value.data();
            int rc = mdb_put(txn, avatars, &lmdbKey, &lmdbData, 0);
            
            if (rc != 0) {
                qDebug() << "Can't change avatar: couldn't store changes to database for" << newAvatar.fileName() << "rolling back to the previous state";
                if (needToRemoveOld) {
                    QFile oldAvatar(currentPath + "/" + res.c_str() + "." + oldInfo.type + ".bak");
                    oldAvatar.rename(currentPath + "/" + res.c_str() + "." + oldInfo.type);
                }
                mdb_txn_abort(txn);
                return false;
            } else {
                mdb_txn_commit(txn);
                if (needToRemoveOld) {
                    QFile oldAvatar(currentPath + "/" + res.c_str() + "." + oldInfo.type + ".bak");
                    oldAvatar.remove();
                }
                return true;
            }
        } else {
            qDebug() << "Can't change avatar: cant open file to write" << newAvatar.fileName() << "rolling back to the previous state";
            if (needToRemoveOld) {
                QFile oldAvatar(currentPath + "/" + res.c_str() + "." + oldInfo.type + ".bak");
                oldAvatar.rename(currentPath + "/" + res.c_str() + "." + oldInfo.type);
            }
            return false;
        }
    }
}

bool Core::Archive::readAvatarInfo(Core::Archive::AvatarInfo& target, const QString& resource) const
{
    if (!opened) {
        throw Closed("readAvatarInfo", jid.toStdString());
    }
    std::string res = resource.size() == 0 ? jid.toStdString() : resource.toStdString();
    
    MDB_txn *txn;
    mdb_txn_begin(environment, NULL, MDB_RDONLY, &txn);
    
    try {
        bool success = readAvatarInfo(target, res, txn);
        mdb_txn_abort(txn);
        return success;
    } catch (...) {
        mdb_txn_abort(txn);
        throw;
    }
    
}

bool Core::Archive::readAvatarInfo(Core::Archive::AvatarInfo& target, const std::string& res, MDB_txn* txn) const
{
    MDB_val lmdbKey, lmdbData;
    lmdbKey.mv_size = res.size();
    lmdbKey.mv_data = (char*)res.c_str();
    
    int rc;
    rc = mdb_get(txn, avatars, &lmdbKey, &lmdbData);
    if (rc == MDB_NOTFOUND) {
        return false;
    } else if (rc) {
        std::string err(mdb_strerror(rc));
        qDebug() << "error reading avatar info for" << res.c_str() << "resource of" << jid << err.c_str();
        throw Unknown(jid.toStdString(), err);
    } else {
        target.deserialize((char*)lmdbData.mv_data, lmdbData.mv_size);
        return true;
    }
}

void Core::Archive::readAllResourcesAvatars(std::map<QString, AvatarInfo>& data) const
{
    if (!opened) {
        throw Closed("readAllResourcesAvatars", jid.toStdString());
    }
    
    int rc;
    MDB_val lmdbKey, lmdbData;
    MDB_txn *txn;
    MDB_cursor* cursor;
    mdb_txn_begin(environment, NULL, MDB_RDONLY, &txn);
    mdb_cursor_open(txn, avatars, &cursor);
    rc = mdb_cursor_get(cursor, &lmdbKey, &lmdbData, MDB_FIRST);
    if (rc == 0) {                                                  //the db might be empty yet
        do {
            std::string sId((char*)lmdbKey.mv_data, lmdbKey.mv_size);
            QString res(sId.c_str());
            if (res != jid) {
                data.emplace(res, AvatarInfo());
                data[res].deserialize((char*)lmdbData.mv_data, lmdbData.mv_size);
            }
        } while (mdb_cursor_get(cursor, &lmdbKey, &lmdbData, MDB_NEXT) == 0);
    }
    
    mdb_cursor_close(cursor);
    mdb_txn_abort(txn);
}

Core::Archive::AvatarInfo Core::Archive::getAvatarInfo(const QString& resource) const
{
    if (!opened) {
        throw Closed("readAvatarInfo", jid.toStdString());
    }
    
    AvatarInfo info;
    bool success = readAvatarInfo(info, resource);
    if (success) {
        return info;
    } else {
        throw NoAvatar(jid.toStdString(), resource.toStdString());
    }
}

Core::Archive::AvatarInfo::AvatarInfo():
type(),
hash(),
autogenerated(false) {}

Core::Archive::AvatarInfo::AvatarInfo(const QString& p_type, const QByteArray& p_hash, bool p_autogenerated):
type(p_type),
hash(p_hash),
autogenerated(p_autogenerated) {}

void Core::Archive::AvatarInfo::deserialize(char* pointer, uint32_t size)
{
    QByteArray data = QByteArray::fromRawData(pointer, size);
    QDataStream in(&data, QIODevice::ReadOnly);
    
    in >> type >> hash >> autogenerated;
}

void Core::Archive::AvatarInfo::serialize(QByteArray* ba) const
{
    QDataStream out(ba, QIODevice::WriteOnly);
    
    out << type << hash << autogenerated;
}
