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

#ifndef CORE_URLSTORAGE_H
#define CORE_URLSTORAGE_H

#include <QString>
#include <QDataStream>
#include <lmdb.h>
#include <list>

#include "archive.h"

namespace Core {

/**
 * @todo write docs
 */
class UrlStorage
{
    class UrlInfo;
public:
    struct MessageInfo {
        MessageInfo();
        MessageInfo(const QString& acc, const QString& j, const QString& id);
        
        QString account;
        QString jid;
        QString messageId;
    };
    
    UrlStorage(const QString& name);
    ~UrlStorage();
    
    void open();
    void close();
    
    void addFile(const QString& url);
    void addFile(const QString& url, const QString& path);
    void addFile(const QString& url, const QString& account, const QString& jid, const QString& id);
    void addFile(const QString& url, const QString& path, const QString& account, const QString& jid, const QString& id);
    std::list<MessageInfo> removeFile(const QString& url);      //removes entry like it never was in the database, returns affected message infos
    std::list<MessageInfo> deletedFile(const QString& path);    //empties the localPath of the entry, returns affected message infos
    std::list<MessageInfo> setPath(const QString& url, const QString& path);
    QString addMessageAndCheckForPath(const QString& url, const QString& account, const QString& jid, const QString& id);
    
private:
    QString name;
    bool opened;
    MDB_env* environment;
    MDB_dbi base;
    MDB_dbi map;
    
private:
    void writeInfo(const QString& key, const UrlInfo& info, bool overwrite = false);
    void writeInfo(const QString& key, const UrlInfo& info, MDB_txn* txn, bool overwrite = false);
    void readInfo(const QString& key, UrlInfo& info);
    void readInfo(const QString& key, UrlInfo& info, MDB_txn* txn);
    
private:
    class UrlInfo {
    public:
        UrlInfo(const QString& path);
        UrlInfo();
        ~UrlInfo();
        
        void serialize(QDataStream& data) const;
        void deserialize(QDataStream& data);
        
        QString getPath() const;
        bool hasPath() const;
        void setPath(const QString& path);
        
        void addMessage(const QString& acc, const QString& jid, const QString& id);
        void getMessages(std::list<MessageInfo>& container) const;
        
    private:
        QString localPath;
        std::list<MessageInfo> messages;
    };
    
    
};

}

#endif // CORE_URLSTORAGE_H
