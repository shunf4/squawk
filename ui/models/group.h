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

#ifndef MODELS_GROUP_H
#define MODELS_GROUP_H

#include "item.h"

namespace Models {

class Contact;
    
class Group : public Models::Item
{
    Q_OBJECT
public:
    Group(const QMap<QString, QVariant> &data, Item *parentItem = 0);
    ~Group();
    
    void appendChild(Models::Item* child) override;
    int columnCount() const override;
    QVariant data(int column) const override;
    
    unsigned int getUnreadMessages() const;
    unsigned int getOnlineContacts() const;

protected:
    void _removeChild(int index) override;
    void setUnreadMessages(unsigned int amount);
    
private slots:
    void refresh();
    
private:
    unsigned int unreadMessages;

};

}

#endif // MODELS_GROUP_H
