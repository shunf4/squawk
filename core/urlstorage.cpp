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
#include <QDebug>

#include "urlstorage.h"

Core::UrlStorage::UrlStorage(const QString& p_name):
    name(p_name),
    opened(false),
    environment(),
    base(),
    map()
{
}

Core::UrlStorage::~UrlStorage()
{
    close();
}

void Core::UrlStorage::open()
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
        
        mdb_env_set_maxdbs(environment, 2);
        mdb_env_set_mapsize(environment, 10UL * 1024UL * 1024UL);
        mdb_env_open(environment, path.toStdString().c_str(), 0, 0664);
        
        MDB_txn *txn;
        mdb_txn_begin(environment, NULL, 0, &txn);
        mdb_dbi_open(txn, "base", MDB_CREATE, &base);
        mdb_dbi_open(txn, "map", MDB_CREATE, &map);
        mdb_txn_commit(txn);
        opened = true;
    }
}

void Core::UrlStorage::close()
{
    if (opened) {
        mdb_dbi_close(environment, map);
        mdb_dbi_close(environment, base);
        mdb_env_close(environment);
        opened = false;
    }
}

void Core::UrlStorage::writeInfo(const QString& key, const Core::UrlStorage::UrlInfo& info, bool overwrite)
{
    MDB_txn *txn;
    mdb_txn_begin(environment, NULL, 0, &txn);
    
    try {
        writeInfo(key, info, txn, overwrite);
        mdb_txn_commit(txn);
    } catch (...) {
        mdb_txn_abort(txn);
        throw;
    }
}

void Core::UrlStorage::writeInfo(const QString& key, const Core::UrlStorage::UrlInfo& info, MDB_txn* txn, bool overwrite)
{
    QByteArray ba;
    QDataStream ds(&ba, QIODevice::WriteOnly);
    info.serialize(ds);
    
    const std::string& id = key.toStdString();
    MDB_val lmdbKey, lmdbData;
    lmdbKey.mv_size = id.size();
    lmdbKey.mv_data = (char*)id.c_str();
    lmdbData.mv_size = ba.size();
    lmdbData.mv_data = (uint8_t*)ba.data();
    
    int rc;
    rc = mdb_put(txn, base, &lmdbKey, &lmdbData, overwrite ? 0 : MDB_NOOVERWRITE);
    
    if (rc != 0) {
        if (rc == MDB_KEYEXIST) {
            if (!overwrite) {
                throw Archive::Exist(name.toStdString(), id);
            }
        } else {
            throw Archive::Unknown(name.toStdString(), mdb_strerror(rc));
        }
    }
    
    if (info.hasPath()) {
        std::string sp = info.getPath().toStdString();
        lmdbData.mv_size = sp.size();
        lmdbData.mv_data = (char*)sp.c_str();
        rc = mdb_put(txn, map, &lmdbData, &lmdbKey, 0);
        if (rc != 0) {
            throw Archive::Unknown(name.toStdString(), mdb_strerror(rc));
        }
    }
}

void Core::UrlStorage::readInfo(const QString& key, Core::UrlStorage::UrlInfo& info, MDB_txn* txn)
{
    const std::string& id = key.toStdString();
    MDB_val lmdbKey, lmdbData;
    lmdbKey.mv_size = id.size();
    lmdbKey.mv_data = (char*)id.c_str();
    int rc = mdb_get(txn, base, &lmdbKey, &lmdbData);
    
    if (rc == 0) {
        QByteArray ba((char*)lmdbData.mv_data, lmdbData.mv_size);
        QDataStream ds(&ba, QIODevice::ReadOnly);
        
        info.deserialize(ds);
    } else if (rc == MDB_NOTFOUND) {
        throw Archive::NotFound(id, name.toStdString());
    } else {
        throw Archive::Unknown(name.toStdString(), mdb_strerror(rc));
    }
}

void Core::UrlStorage::readInfo(const QString& key, Core::UrlStorage::UrlInfo& info)
{
    MDB_txn *txn;
    mdb_txn_begin(environment, NULL, MDB_RDONLY, &txn);
    
    try {
        readInfo(key, info, txn);
        mdb_txn_commit(txn);
    } catch (...) {
        mdb_txn_abort(txn);
        throw;
    }
}

void Core::UrlStorage::addFile(const QString& url)
{
    if (!opened) {
        throw Archive::Closed("addFile(no message, no path)", name.toStdString());
    }
    
    addToInfo(url, "", "", "");
}

void Core::UrlStorage::addFile(const QString& url, const QString& path)
{
    if (!opened) {
        throw Archive::Closed("addFile(no message, with path)", name.toStdString());
    }
    
    addToInfo(url, "", "", "", path);
}

void Core::UrlStorage::addFile(const QString& url, const QString& account, const QString& jid, const QString& id)
{
    if (!opened) {
        throw Archive::Closed("addFile(with message, no path)", name.toStdString());
    }
    
    addToInfo(url, account, jid, id);
}

void Core::UrlStorage::addFile(const QString& url, const QString& path, const QString& account, const QString& jid, const QString& id)
{
    if (!opened) {
        throw Archive::Closed("addFile(with message, with path)", name.toStdString());
    }
    
    addToInfo(url, account, jid, id, path);
}

void Core::UrlStorage::addFile(const std::list<Shared::MessageInfo>& msgs, const QString& url, const QString& path)
{
    if (!opened) {
        throw Archive::Closed("addFile(with list)", name.toStdString());
    }
    
    UrlInfo info (path, msgs);
    writeInfo(url, info, true);;
}

QString Core::UrlStorage::addMessageAndCheckForPath(const QString& url, const QString& account, const QString& jid, const QString& id)
{
    if (!opened) {
        throw Archive::Closed("addMessageAndCheckForPath", name.toStdString());
    }
    
    return addToInfo(url, account, jid, id).getPath();
}

Core::UrlStorage::UrlInfo Core::UrlStorage::addToInfo(const QString& url, const QString& account, const QString& jid, const QString& id, const QString& path)
{
    UrlInfo info;
    MDB_txn *txn;
    mdb_txn_begin(environment, NULL, 0, &txn);
    
    try {
        readInfo(url, info, txn);
    } catch (const Archive::NotFound& e) {
        
    } catch (...) {
        mdb_txn_abort(txn);
        throw;
    }
    
    bool pathChange = false;
    bool listChange = false;
    if (path != "-s") {
        if (info.getPath() != path) {
            info.setPath(path);
            pathChange = true;
        }
    }
    
    if (account.size() > 0 && jid.size() > 0 && id.size() > 0) {
        listChange = info.addMessage(account, jid, id);
    }
    
    if (pathChange || listChange) {
        try {
            writeInfo(url, info, txn, true);
            mdb_txn_commit(txn);
        } catch (...) {
            mdb_txn_abort(txn);
            throw;
        }
    } else {
        mdb_txn_abort(txn);
    }
    
    return info;
}

std::list<Shared::MessageInfo> Core::UrlStorage::setPath(const QString& url, const QString& path)
{
    std::list<Shared::MessageInfo> list;
    
    MDB_txn *txn;
    mdb_txn_begin(environment, NULL, 0, &txn);
    UrlInfo info;
    
    try {
        readInfo(url, info, txn);
        info.getMessages(list);
    } catch (const Archive::NotFound& e) {
    } catch (...) {
        mdb_txn_abort(txn);
        throw;
    }
    
    info.setPath(path);
    try {
        writeInfo(url, info, txn, true);
        mdb_txn_commit(txn);
    } catch (...) {
        mdb_txn_abort(txn);
        throw;
    }
    
    return list;
}

std::list<Shared::MessageInfo> Core::UrlStorage::removeFile(const QString& url)
{
    std::list<Shared::MessageInfo> list;
    
    MDB_txn *txn;
    mdb_txn_begin(environment, NULL, 0, &txn);
    UrlInfo info;
    
    try {
        std::string id = url.toStdString();
        readInfo(url, info, txn);
        info.getMessages(list);
        
        MDB_val lmdbKey;
        lmdbKey.mv_size = id.size();
        lmdbKey.mv_data = (char*)id.c_str();
        int rc = mdb_del(txn, base, &lmdbKey, NULL);
        if (rc != 0) {
            throw Archive::Unknown(name.toStdString(), mdb_strerror(rc));
        }
        
        if (info.hasPath()) {
            std::string path = info.getPath().toStdString();
            lmdbKey.mv_size = path.size();
            lmdbKey.mv_data = (char*)path.c_str();
            
            int rc = mdb_del(txn, map, &lmdbKey, NULL);
            if (rc != 0) {
                throw Archive::Unknown(name.toStdString(), mdb_strerror(rc));
            }
        }
        mdb_txn_commit(txn);
    } catch (...) {
        mdb_txn_abort(txn);
        throw;
    }
    
    return list;
}

std::list<Shared::MessageInfo> Core::UrlStorage::deletedFile(const QString& path)
{
    std::list<Shared::MessageInfo> list;
    
    MDB_txn *txn;
    mdb_txn_begin(environment, NULL, 0, &txn);
    
    try {
        std::string spath = path.toStdString();
        
        MDB_val lmdbKey, lmdbData;
        lmdbKey.mv_size = spath.size();
        lmdbKey.mv_data = (char*)spath.c_str();
        
        QString url;
        int rc = mdb_get(txn, map, &lmdbKey, &lmdbData);
        
        if (rc == 0) {
            std::string surl((char*)lmdbData.mv_data, lmdbData.mv_size);
            url = QString(surl.c_str());
        } else if (rc == MDB_NOTFOUND) {
            qDebug() << "Have been asked to remove file" << path << ", which isn't in the database, skipping";
            mdb_txn_abort(txn);
            return list;
        } else {
            throw Archive::Unknown(name.toStdString(), mdb_strerror(rc));
        }
        
        UrlInfo info;
        std::string id = url.toStdString();
        readInfo(url, info, txn);
        info.getMessages(list);
        info.setPath(QString());
        writeInfo(url, info, txn, true);
        
        rc = mdb_del(txn, map, &lmdbKey, NULL);
        if (rc != 0) {
            throw Archive::Unknown(name.toStdString(), mdb_strerror(rc));
        }
        
        mdb_txn_commit(txn);
    } catch (...) {
        mdb_txn_abort(txn);
        throw;
    }
    
    return list;
}


QString Core::UrlStorage::getUrl(const QString& path)
{
    std::list<Shared::MessageInfo> list;
    
    MDB_txn *txn;
    mdb_txn_begin(environment, NULL, MDB_RDONLY, &txn);
    
    std::string spath = path.toStdString();
    
    MDB_val lmdbKey, lmdbData;
    lmdbKey.mv_size = spath.size();
    lmdbKey.mv_data = (char*)spath.c_str();
    
    QString url;
    int rc = mdb_get(txn, map, &lmdbKey, &lmdbData);
    
    if (rc == 0) {
        std::string surl((char*)lmdbData.mv_data, lmdbData.mv_size);
        url = QString(surl.c_str());
        
        mdb_txn_abort(txn);
        return url;
    } else if (rc == MDB_NOTFOUND) {
        mdb_txn_abort(txn);
        throw Archive::NotFound(spath, name.toStdString());
    } else {
        mdb_txn_abort(txn);
        throw Archive::Unknown(name.toStdString(), mdb_strerror(rc));
    }
}

std::pair<QString, std::list<Shared::MessageInfo>> Core::UrlStorage::getPath(const QString& url)
{
    UrlInfo info;
    readInfo(url, info);
    std::list<Shared::MessageInfo> container;
    info.getMessages(container);
    return std::make_pair(info.getPath(), container);
}

Core::UrlStorage::UrlInfo::UrlInfo():
    localPath(),
    messages() {}

Core::UrlStorage::UrlInfo::UrlInfo(const QString& path):
    localPath(path),
    messages() {}
 
Core::UrlStorage::UrlInfo::UrlInfo(const QString& path, const std::list<Shared::MessageInfo>& msgs):
    localPath(path),
    messages(msgs) {}
 
Core::UrlStorage::UrlInfo::~UrlInfo() {}
 
bool Core::UrlStorage::UrlInfo::addMessage(const QString& acc, const QString& jid, const QString& id)
{
    for (const Shared::MessageInfo& info : messages) {
        if (info.account == acc && info.jid == jid && info.messageId == id) {
            return false;
        }
    }
    messages.emplace_back(acc, jid, id);
    return true;
}

void Core::UrlStorage::UrlInfo::serialize(QDataStream& data) const
{
    data << localPath;
    std::list<Shared::MessageInfo>::size_type size = messages.size();
    data << quint32(size);
    for (const Shared::MessageInfo& info : messages) {
        data << info.account;
        data << info.jid;
        data << info.messageId;
    }
}

void Core::UrlStorage::UrlInfo::deserialize(QDataStream& data)
{
    data >> localPath;
    quint32 size;
    data >> size;
    for (quint32 i = 0; i < size; ++i) {
        messages.emplace_back();
        Shared::MessageInfo& info = messages.back();
        data >> info.account;
        data >> info.jid;
        data >> info.messageId;
    }
}

void Core::UrlStorage::UrlInfo::getMessages(std::list<Shared::MessageInfo>& container) const
{
    for (const Shared::MessageInfo& info : messages) {
        container.emplace_back(info);
    }
}

QString Core::UrlStorage::UrlInfo::getPath() const
{
    return localPath;
}

bool Core::UrlStorage::UrlInfo::hasPath() const
{
    return localPath.size() > 0;
}


void Core::UrlStorage::UrlInfo::setPath(const QString& path)
{
    localPath = path;
}
