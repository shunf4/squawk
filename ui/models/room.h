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

#ifndef MODELS_ROOM_H
#define MODELS_ROOM_H

#include "item.h"
#include "../global.h"

namespace Models {

/**
 * @todo write docs
 */
class Room : public Models::Item
{
    Q_OBJECT
public:
    typedef std::deque<Shared::Message> Messages;
    Room(const QString& p_jid, const QMap<QString, QVariant> &data, Item *parentItem = 0);
    ~Room();
    
    int columnCount() const override;
    QVariant data(int column) const override;
    
    unsigned int getUnreadMessagesCount() const;
    bool getJoined() const;
    bool getAutoJoin() const;
    QString getJid() const;
    QString getNick() const;
    QString getRoomName() const;
    
    QIcon getStatusIcon(bool big = false) const;
    QString getStatusText() const;
    
    void setJoined(bool p_joined);
    void setAutoJoin(bool p_autoJoin);
    void setJid(const QString& p_jid);
    void setNick(const QString& p_nick);
    
    void update(const QString& field, const QVariant& value);
    
    void addMessage(const Shared::Message& data);
    unsigned int getMessagesCount() const;
    void dropMessages();
    void getMessages(Messages& container) const;

protected:
    
private:
    bool autoJoin;
    bool joined;
    QString jid;
    QString nick;
    Messages messages;

};

}

#endif // MODELS_ROOM_H
