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

#ifndef MODELS_CONTACT_H
#define MODELS_CONTACT_H

#include "element.h"
#include "presence.h"
#include "shared/enums.h"
#include "shared/message.h"
#include "shared/icons.h"
#include "shared/global.h"

#include <QMap>
#include <QIcon>
#include <deque>

namespace Models {
    
class Contact : public Element
{
    Q_OBJECT
public:
    Contact(const Account* acc, const QString& p_jid, const QMap<QString, QVariant> &data, Item *parentItem = 0);
    ~Contact();
    
    Shared::Availability getAvailability() const;
    Shared::SubscriptionState getState() const;

    QIcon getStatusIcon(bool big = false) const;
    
    int columnCount() const override;
    QVariant data(int column) const override;
        
    void update(const QString& field, const QVariant& value) override;
    
    void addPresence(const QString& name, const QMap<QString, QVariant>& data);
    void removePresence(const QString& name);
    
    QString getContactName() const;
    QString getStatus() const;
    QString getDisplayedName() const override;
    
protected:
    void _removeChild(int index) override;
    void _appendChild(Models::Item * child) override;
    
protected slots:
    void refresh();
    void toOfflineState() override;
    
protected:
    void setAvailability(Shared::Availability p_state);
    void setAvailability(unsigned int p_state);
    void setState(Shared::SubscriptionState p_state);
    void setState(unsigned int p_state);
    void setStatus(const QString& p_state);
    
private:
    Shared::Availability availability;
    Shared::SubscriptionState state;
    QMap<QString, Presence*> presences;
    QString status;
};

}

#endif // MODELS_CONTACT_H
