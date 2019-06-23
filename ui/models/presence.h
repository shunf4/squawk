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

#ifndef MODELS_PRESENCE_H
#define MODELS_PRESENCE_H

#include "item.h"
#include "../../global.h"
#include <QDateTime>
#include <QIcon>

namespace Models {

class Presence : public Models::Item
{
    Q_OBJECT
public:
    typedef std::deque<Shared::Message> Messages;
    explicit Presence(const QMap<QString, QVariant> &data, Item *parentItem = 0);
    ~Presence();

    virtual int columnCount() const override;
    virtual QVariant data(int column) const override;
    
    Shared::Availability getAvailability() const;
    void setAvailability(Shared::Availability p_avail);
    void setAvailability(unsigned int avail);
    
    QDateTime getLastActivity() const;
    void setLastActivity(const QDateTime& p_time);
    
    QString getStatus() const;
    void setStatus(const QString& p_state);
    QIcon getStatusIcon(bool big = false) const;
    
    void update(const QString& key, const QVariant& value);
    unsigned int getMessagesCount() const;
    void dropMessages();
    void addMessage(const Shared::Message& data);
    
    void getMessages(Messages& container) const;

private:
    Shared::Availability availability;
    QDateTime lastActivity;
    QString status;
    Messages messages;
};

}

#endif // MODELS_PRESENCE_H
