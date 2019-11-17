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
    hasAvatar(false),
    avatarAutoGenerated(false),
    avatarHash(),
    avatarType()
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
        
        mdb_env_set_maxdbs(environment, 4);
        mdb_env_set_mapsize(environment, 512UL * 1024UL * 1024UL);
        mdb_env_open(environment, path.toStdString().c_str(), 0, 0664);
        
        MDB_txn *txn;
        mdb_txn_begin(environment, NULL, 0, &txn);
        mdb_dbi_open(txn, "main", MDB_CREATE, &main);
        mdb_dbi_open(txn, "order", MDB_CREATE | MDB_INTEGERKEY, &order);
        mdb_dbi_open(txn, "stats", MDB_CREATE, &stats);
        mdb_txn_commit(txn);
        
        mdb_txn_begin(environment, NULL, 0, &txn);
        try {
            fromTheBeginning = getStatBoolValue("beginning", txn);
        } catch (const NotFound& e) {
            fromTheBeginning = false;
        }
        try {
            hasAvatar = getStatBoolValue("hasAvatar", txn);
        } catch (const NotFound& e) {
            hasAvatar = false;
        }
        if (hasAvatar) {
            try {
                avatarAutoGenerated = getStatBoolValue("avatarAutoGenerated", txn);
            } catch (const NotFound& e) {
                avatarAutoGenerated = false;
            }
            
            avatarType = getStatStringValue("avatarType", txn).c_str();
            if (avatarAutoGenerated) {
                avatarHash = "";
            } else {
                avatarHash = getStatStringValue("avatarHash", txn).c_str();
            }
        } else {
            avatarAutoGenerated = false;
            avatarHash = "";
            avatarType = "";
        }
        mdb_txn_abort(txn);
        
        if (hasAvatar) {
            QFile ava(path + "/avatar." + avatarType);
            if (!ava.exists()) {
                bool success = dropAvatar();
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
    mdb_drop(txn, stats, 0);
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
        throw Empty(jid.toStdString());
    } else {
        std::string sId((char*)lmdbData.mv_data, lmdbData.mv_size);
        mdb_cursor_close(cursor);
        mdb_txn_abort(txn);
        return sId.c_str();
    }
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
        throw Empty(jid.toStdString());
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
                //qDebug() << "element added with id" << message.getId() << "stamp" << message.getTime();
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
    if (id == "") {
        rc = mdb_cursor_open(txn, order, &cursor);
        rc = mdb_cursor_get(cursor, &lmdbKey, &lmdbData, MDB_LAST);
        if (rc) {
            qDebug() << "Error getting before" << mdb_strerror(rc) << ", id:" << id;
            mdb_cursor_close(cursor);
            mdb_txn_abort(txn);
            
            throw Empty(jid.toStdString());
        }
    } else {
        std::string stdId(id.toStdString());
        lmdbKey.mv_size = stdId.size();
        lmdbKey.mv_data = (char*)stdId.c_str();
        rc = mdb_get(txn, main, &lmdbKey, &lmdbData);
        if (rc) {
            qDebug() <<"Error getting before: no reference message" << mdb_strerror(rc) << ", id:" << id;
            mdb_txn_abort(txn);
            throw NotFound(stdId, jid.toStdString());
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
                qDebug() << "Error getting before: couldn't set " << mdb_strerror(rc);
                mdb_cursor_close(cursor);
                mdb_txn_abort(txn);
                throw NotFound(stdId, jid.toStdString());
            } else {
                rc = mdb_cursor_get(cursor, &lmdbKey, &lmdbData, MDB_PREV);
                if (rc) {
                    qDebug() << "Error getting before, couldn't prev " << mdb_strerror(rc);
                    mdb_cursor_close(cursor);
                    mdb_txn_abort(txn);
                    throw NotFound(stdId, jid.toStdString());
                }
            }
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
        qDebug() << "error retrieving" << id.c_str() << "from stats db of" << jid << mdb_strerror(rc);
        throw 15;            //TODO proper exception
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
        qDebug() << "error retrieving" << id.c_str() << "from stats db of" << jid << mdb_strerror(rc);
        throw 15;            //TODO proper exception
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

bool Core::Archive::getHasAvatar() const
{
    if (!opened) {
        throw Closed("getHasAvatar", jid.toStdString());
    }
    
    return hasAvatar;
}

bool Core::Archive::getAutoAvatar() const
{
    if (!opened) {
        throw Closed("getAutoAvatar", jid.toStdString());
    }
    
    return avatarAutoGenerated;
}

QString Core::Archive::getAvatarHash() const
{
    if (!opened) {
        throw Closed("getAvatarHash", jid.toStdString());
    }
    
    return avatarHash;
}

QString Core::Archive::getAvatarType() const
{
    if (!opened) {
        throw Closed("getAvatarType", jid.toStdString());
    }
    
    return avatarType;
}

bool Core::Archive::dropAvatar()
{
    MDB_txn *txn;
    mdb_txn_begin(environment, NULL, 0, &txn);
    bool success = setStatValue("hasAvatar", false, txn);
    success = success && setStatValue("avatarAutoGenerated", false, txn);
    success = success && setStatValue("avatarHash", "", txn);
    success = success && setStatValue("avatarType", "", txn);
    if (!success) {
        mdb_txn_abort(txn);
        return false;
    } else {
        hasAvatar = false;
        avatarAutoGenerated = false;
        avatarHash = "";
        avatarType = "";
        mdb_txn_commit(txn);
        return true;
    }
}

bool Core::Archive::setAvatar(const QByteArray& data, bool generated)
{
    if (!opened) {
        throw Closed("setAvatar", jid.toStdString());
    }
    
    if (data.size() == 0) {
        if (!hasAvatar) {
            return false;
        } else {
            return dropAvatar();
        }
    } else {
        const char* cep;
        mdb_env_get_path(environment, &cep);
        QString currentPath(cep);
        bool needToRemoveOld = false;
        QCryptographicHash hash(QCryptographicHash::Sha1);
        hash.addData(data);
        QString newHash(hash.result());
        if (hasAvatar) {
            if (!generated && !avatarAutoGenerated && avatarHash == newHash) {
                return false;
            }
            QFile oldAvatar(currentPath + "/avatar." + avatarType);
            if (oldAvatar.exists()) {
                if (oldAvatar.rename(currentPath + "/avatar." + avatarType + ".bak")) {
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
        QFile newAvatar(currentPath + "/avatar." + ext);
        if (newAvatar.open(QFile::WriteOnly)) {
            newAvatar.write(data);
            newAvatar.close();
            
            MDB_txn *txn;
            mdb_txn_begin(environment, NULL, 0, &txn);
            bool success = setStatValue("hasAvatar", true, txn);
            success = success && setStatValue("avatarAutoGenerated", generated, txn);
            success = success && setStatValue("avatarHash", newHash.toStdString(), txn);
            success = success && setStatValue("avatarType", ext.toStdString(), txn);
            if (!success) {
                qDebug() << "Can't change avatar: couldn't store changes to database for" << newAvatar.fileName() << "rolling back to the previous state";
                if (needToRemoveOld) {
                    QFile oldAvatar(currentPath + "/avatar." + avatarType + ".bak");
                    oldAvatar.rename(currentPath + "/avatar." + avatarType);
                }
                mdb_txn_abort(txn);
                return false;
            } else {
                hasAvatar = true;
                avatarAutoGenerated = generated;
                avatarHash = newHash;
                avatarType = ext;
                mdb_txn_commit(txn);
                if (needToRemoveOld) {
                    QFile oldAvatar(currentPath + "/avatar." + avatarType + ".bak");
                    oldAvatar.remove();
                }
                return true;
            }
        } else {
            qDebug() << "Can't change avatar: cant open file to write" << newAvatar.fileName() << "rolling back to the previous state";
            if (needToRemoveOld) {
                QFile oldAvatar(currentPath + "/avatar." + avatarType + ".bak");
                oldAvatar.rename(currentPath + "/avatar." + avatarType);
            }
            return false;
        }
    }
}
