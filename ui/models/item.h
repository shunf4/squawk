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

#include "../../global.h"

namespace Models {

class Item : public QObject{
    Q_OBJECT
    public:
        enum Type {
            account,
            group,
            contact,
            room,
            presence,
            participant,
            root
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
        
        Item *child(int row);
        int childCount() const;
        virtual int columnCount() const;
        virtual QVariant data(int column) const;
        int row() const;
        Item *parentItem();
        const Item *parentItemConst() const;
        
        
        QString getAccountName() const;
        QString getAccountJid() const;
        QString getAccountResource() const;
        Shared::ConnectionState getAccountConnectionState() const;
        Shared::Availability getAccountAvailability() const;
        
        const Type type;
        
    protected:
        virtual void changed(int col);
        virtual void _removeChild(int index);
        virtual bool columnInvolvedInDisplay(int col);
        const Item* getParentAccount() const;
        
    protected slots:
        void onChildChanged(Models::Item* item, int row, int col);
        
    protected:
        QString name;
        std::deque<Item*> childItems;
        Item* parent;
        
    protected slots:
        virtual void toOfflineState();
        
    };

}

#endif // MODELS_ITEM_H
