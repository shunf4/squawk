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

#include "abstractparticipant.h"

using namespace Models;

Models::AbstractParticipant::AbstractParticipant(Models::Item::Type p_type, const QMap<QString, QVariant>& data, Models::Item* parentItem):
    Item(p_type, data, parentItem),
    availability(Shared::offline),
    lastActivity(data.value("lastActivity").toDateTime()),
    status(data.value("status").toString())
{
    QMap<QString, QVariant>::const_iterator itr = data.find("availability");
    if (itr != data.end()) {
        setAvailability(itr.value().toUInt());
    }
}

Models::AbstractParticipant::AbstractParticipant(const Models::AbstractParticipant& other):
    Item(other),
    availability(other.availability),
    lastActivity(other.lastActivity),
    status(other.status)
{
}


Models::AbstractParticipant::~AbstractParticipant()
{
}

int Models::AbstractParticipant::columnCount() const
{
    return 4;
}

QVariant Models::AbstractParticipant::data(int column) const
{
    switch (column) {
        case 0:
            return Item::data(column);
        case 1:
            return lastActivity;
        case 2:
            return availability;
        case 3:
            return status;
        default:
            return QVariant();
    }
}

Shared::Availability Models::AbstractParticipant::getAvailability() const
{
    return availability;
}

QDateTime Models::AbstractParticipant::getLastActivity() const
{
    return lastActivity;
}

QString Models::AbstractParticipant::getStatus() const
{
    return status;
}

void Models::AbstractParticipant::setAvailability(Shared::Availability p_avail)
{
    if (availability != p_avail) {
        availability = p_avail;
        changed(2);
    }
}

void Models::AbstractParticipant::setAvailability(unsigned int avail)
{
    if (avail <= Shared::availabilityHighest) {
        Shared::Availability state = static_cast<Shared::Availability>(avail);
        setAvailability(state);
    } else {
        qDebug("An attempt to set wrong state to the contact");
    }
}


void Models::AbstractParticipant::setLastActivity(const QDateTime& p_time)
{
    if (lastActivity != p_time) {
        lastActivity = p_time;
        changed(1);
    }
}

void Models::AbstractParticipant::setStatus(const QString& p_state)
{
    if (status != p_state) {
        status = p_state;
        changed(3);
    }
}

QIcon Models::AbstractParticipant::getStatusIcon(bool big) const
{
    return Shared::availabilityIcon(availability, big);
}

void Models::AbstractParticipant::update(const QString& key, const QVariant& value)
{
    if (key == "name") {
        setName(value.toString());
    } else if (key == "status") {
        setStatus(value.toString());
    } else if (key == "availability") {
        setAvailability(value.toUInt());
    } else if (key == "lastActivity") {
        setLastActivity(value.toDateTime());
    }
}
