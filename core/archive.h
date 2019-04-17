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

#ifndef CORE_ARCHIVE_H
#define CORE_ARCHIVE_H

#include <QObject>
#include "../global.h"
#include <lmdb.h>
#include "../exception.h"

namespace Core {

class Archive : public QObject
{
    Q_OBJECT
public:
    Archive(const QString& jid, QObject* parent = 0);
    ~Archive();
    
    void open(const QString& account);
    void close();
    
    void addElement(const Shared::Message& message);
    Shared::Message getElement(const QString& id);
    Shared::Message oldest();
    QString oldestId();
    Shared::Message newest();
    QString newestId();
    void clear();
    long unsigned int size() const;
    
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
    
private:
    bool opened;
    MDB_env* environment;
    MDB_dbi main;
    MDB_dbi order;
};

}

#endif // CORE_ARCHIVE_H
