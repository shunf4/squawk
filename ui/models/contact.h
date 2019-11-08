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

#include "item.h"
#include "presence.h"
#include "../../global.h"
#include <QMap>
#include <QIcon>
#include <deque>

namespace Models {
    
class Contact : public Item
{
    Q_OBJECT
public:
    typedef std::deque<Shared::Message> Messages;
    Contact(const QString& p_jid, const QMap<QString, QVariant> &data, Item *parentItem = 0);
    Contact(const Contact& other);
    ~Contact();
    
    QString getJid() const;
    Shared::Availability getAvailability() const;
    Shared::SubscriptionState getState() const;
    Shared::Avatar getAvatarState() const;
    QString getAvatarPath() const;
    QIcon getStatusIcon(bool big = false) const;
    
    int columnCount() const override;
    QVariant data(int column) const override;
        
    void update(const QString& field, const QVariant& value);
    
    void addPresence(const QString& name, const QMap<QString, QVariant>& data);
    void removePresence(const QString& name);
    
    void appendChild(Models::Item * child) override;
    QString getContactName() const;
    QString getStatus() const;
    
    void addMessage(const Shared::Message& data);
    unsigned int getMessagesCount() const;
    void dropMessages();
    void getMessages(Messages& container) const;
    QString getDisplayedName() const override;
    
    Contact* copy() const;
    
protected:
    void _removeChild(int index) override;
    bool columnInvolvedInDisplay(int col) override;
    
protected slots:
    void refresh();
    void toOfflineState() override;
    
protected:
    void setAvailability(Shared::Availability p_state);
    void setAvailability(unsigned int p_state);
    void setState(Shared::SubscriptionState p_state);
    void setState(unsigned int p_state);
    void setAvatarState(Shared::Avatar p_state);
    void setAvatarState(unsigned int p_state);
    void setAvatarPath(const QString& path);
    void setJid(const QString p_jid);
    void setStatus(const QString& p_state);
    
private:
    QString jid;
    Shared::Availability availability;
    Shared::SubscriptionState state;
    Shared::Avatar avatarState;
    QMap<QString, Presence*> presences;
    Messages messages;
    unsigned int childMessages;
    QString status;
    QString avatarPath;
};

}

#endif // MODELS_CONTACT_H
