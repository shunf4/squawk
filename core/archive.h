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

#ifndef CORE_ARCHIVE_H
#define CORE_ARCHIVE_H

#include <QObject>
#include <QCryptographicHash>
#include <QMimeDatabase>
#include <QMimeType>

#include "shared/message.h"
#include "shared/exception.h"
#include <lmdb.h>
#include <list>

namespace Core {

class Archive : public QObject
{
    Q_OBJECT
public:
    class AvatarInfo;
    
    Archive(const QString& jid, QObject* parent = 0);
    ~Archive();
    
    void open(const QString& account);
    void close();
    
    bool addElement(const Shared::Message& message);
    unsigned int addElements(const std::list<Shared::Message>& messages);
    Shared::Message getElement(const QString& id) const;
    bool hasElement(const QString& id) const;
    void changeMessage(const QString& id, const QMap<QString, QVariant>& data);
    Shared::Message oldest();
    QString oldestId();
    Shared::Message newest();
    QString newestId();
    void clear();
    long unsigned int size() const;
    std::list<Shared::Message> getBefore(int count, const QString& id);
    bool isFromTheBeginning();
    void setFromTheBeginning(bool is);
    bool setAvatar(const QByteArray& data, AvatarInfo& info, bool generated = false, const QString& resource = "");
    AvatarInfo getAvatarInfo(const QString& resource = "") const;
    bool readAvatarInfo(AvatarInfo& target, const QString& resource = "") const;
    void readAllResourcesAvatars(std::map<QString, AvatarInfo>& data) const;
    QString idByStanzaId(const QString& stanzaId) const;
    QString stanzaIdById(const QString& id) const;
    
public:
    const QString jid;
    
public:
    class Directory: 
    public Utils::Exception
    {
    public:
        Directory(const std::string& p_path):Exception(), path(p_path){}
        
        std::string getMessage() const{return "Can't create directory for database at " + path;}
    private:
        std::string path;
    };
    
    class Closed: 
    public Utils::Exception
    {
    public:
        Closed(const std::string& op, const std::string& acc):Exception(), operation(op), account(acc){}
        
        std::string getMessage() const{return "An attempt to perform operation " + operation + " on closed archive for " + account;}
    private:
        std::string operation;
        std::string account;
    };
    
    class NotFound: 
    public Utils::Exception
    {
    public:
        NotFound(const std::string& k, const std::string& acc):Exception(), key(k), account(acc){}
        
        std::string getMessage() const{return "Element for id " + key + " wasn't found in database " + account;}
    private:
        std::string key;
        std::string account;
    };
    
    class Empty: 
    public Utils::Exception
    {
    public:
        Empty(const std::string& acc):Exception(), account(acc){}
        
        std::string getMessage() const{return "An attempt to read ordered elements from database " + account + " but it's empty";}
    private:
        std::string account;
    };
    
    class Exist: 
    public Utils::Exception
    {
    public:
        Exist(const std::string& acc, const std::string& p_key):Exception(), account(acc), key(p_key){}
        
        std::string getMessage() const{return "An attempt to insert element " + key + " to database " + account + " but it already has an element with given id";}
    private:
        std::string account;
        std::string key;
    };
    
    class NoAvatar: 
    public Utils::Exception
    {
    public:
        NoAvatar(const std::string& el, const std::string& res):Exception(), element(el), resource(res){
            if (resource.size() == 0) {
                resource = "for himself";
            }
        }
        
        std::string getMessage() const{return "Element " + element + " has no avatar for " + resource ;}
    private:
        std::string element;
        std::string resource;
    };
    
    class Unknown:
    public Utils::Exception
    {
    public:
        Unknown(const std::string& acc, const std::string& message):Exception(), account(acc), msg(message){}
        
        std::string getMessage() const{return "Unknown error on database " + account + ": " + msg;}
    private:
        std::string account;
        std::string msg;
    };
    
    
    class AvatarInfo {
    public:
        AvatarInfo();
        AvatarInfo(const QString& type, const QByteArray& hash, bool autogenerated);
        
        void deserialize(char* pointer, uint32_t size);
        void serialize(QByteArray* ba) const;
        
        QString type;
        QByteArray hash;
        bool autogenerated;
    };
    
private:
    bool opened;
    bool fromTheBeginning;
    MDB_env* environment;
    MDB_dbi main;           //id to message
    MDB_dbi order;          //time to id
    MDB_dbi stats;
    MDB_dbi avatars;        
    MDB_dbi sid;            //stanzaId to id
    
    bool getStatBoolValue(const std::string& id, MDB_txn* txn);
    std::string getStatStringValue(const std::string& id, MDB_txn* txn);
    
    bool setStatValue(const std::string& id, bool value, MDB_txn* txn);
    bool setStatValue(const std::string& id, const std::string& value, MDB_txn* txn);
    bool readAvatarInfo(AvatarInfo& target, const std::string& res, MDB_txn* txn) const;
    void printOrder();
    void printKeys();
    bool dropAvatar(const std::string& resource);
    Shared::Message getMessage(const std::string& id, MDB_txn* txn) const;
    Shared::Message getStoredMessage(MDB_txn *txn, MDB_cursor* cursor, MDB_cursor_op op, MDB_val* key, MDB_val* value, int& rc);
    Shared::Message edge(bool end);
};

}

#endif // CORE_ARCHIVE_H
