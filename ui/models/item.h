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

#ifndef MODELS_ITEM_H
#define MODELS_ITEM_H

#include <QMap>
#include <QString>
#include <QVariant>

#include <deque>
#include <set>

#include "shared/enums.h"
namespace Models {

class Reference;
class Contact;
class Account;
    
class Item : public QObject{
    friend class Reference;
    Q_OBJECT
    public:
        enum Type {
            account,
            group,
            contact,
            room,
            presence,
            participant,
            root,
            reference
        };
        
        explicit Item(Type p_type, const QMap<QString, QVariant> &data, Item *parentItem = 0);
        Item(const Item& other);
        ~Item();
        
    signals:
        void childChanged(Models::Item* item, int row, int col);
        void childIsAboutToBeInserted(Item* parent, int first, int last);
        void childInserted();
        void childIsAboutToBeRemoved(Item* parent, int first, int last);
        void childRemoved();
        void childIsAboutToBeMoved(Item* source, int first, int last, Item* destination, int newIndex);
        void childMoved();
        
    public:
        virtual void appendChild(Item *child);
        virtual void removeChild(int index);
        virtual QString getDisplayedName() const;
        QString getName() const;
        void setName(const QString& name);
        
        virtual Item *child(int row);
        virtual int childCount() const;
        virtual int columnCount() const;
        virtual QVariant data(int column) const;
        int row() const;
        Item *parentItem();
        const Item *parentItemConst() const;
        
        
        QString getAccountName() const;
        QString getAccountJid() const;
        QString getAccountResource() const;
        QString getAccountAvatarPath() const;
        Shared::ConnectionState getAccountConnectionState() const;
        Shared::Availability getAccountAvailability() const;
        
        const Type type;
        
        int getContact(const QString& jid) const;
        std::set<Reference*>::size_type referencesCount() const;
        
    protected:
        virtual void changed(int col);
        virtual void _removeChild(int index);
        virtual void _appendChild(Item *child);
        virtual bool columnInvolvedInDisplay(int col);
        virtual const Account* getParentAccount() const;
        void addReference(Reference* ref);
        void removeReference(Reference* ref);
        
    protected slots:
        void onChildChanged(Models::Item* item, int row, int col);
        virtual void toOfflineState();
        
    protected:
        QString name;
        std::deque<Item*> childItems;
        Item* parent;
        std::set<Reference*> references;
        bool destroyingByParent;
        bool destroyingByOriginal;
    };

}

#endif // MODELS_ITEM_H
