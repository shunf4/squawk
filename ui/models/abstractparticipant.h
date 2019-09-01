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

#ifndef MODELS_ABSTRACTPARTICIPANT_H
#define MODELS_ABSTRACTPARTICIPANT_H


#include "item.h"
#include "../../global.h"
#include <QIcon>

namespace Models {

class AbstractParticipant : public Models::Item
{
    Q_OBJECT
public:
    explicit AbstractParticipant(Type p_type, const QMap<QString, QVariant> &data, Item *parentItem = 0);
    ~AbstractParticipant();
    
    virtual int columnCount() const override;
    virtual QVariant data(int column) const override;
    
    Shared::Availability getAvailability() const;
    void setAvailability(Shared::Availability p_avail);
    void setAvailability(unsigned int avail);
    
    QDateTime getLastActivity() const;
    void setLastActivity(const QDateTime& p_time);
    
    QString getStatus() const;
    void setStatus(const QString& p_state);
    virtual QIcon getStatusIcon(bool big = false) const;
    
    virtual void update(const QString& key, const QVariant& value);
    
protected:
    Shared::Availability availability;
    QDateTime lastActivity;
    QString status;
};

}

#endif // MODELS_ABSTRACTPARTICIPANT_H
