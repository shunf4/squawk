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

#include "participant.h"

#include <QDebug>

Models::Participant::Participant(const QMap<QString, QVariant>& data, Models::Item* parentItem):
    AbstractParticipant(participant, data, parentItem),
    affiliation(Shared::Affiliation::unspecified),
    role(Shared::Role::unspecified)
{
    QMap<QString, QVariant>::const_iterator itr = data.find("affiliation");
    if (itr != data.end()) {
        setAffiliation(itr.value().toUInt());
    }
    
    itr = data.find("role");
    if (itr != data.end()) {
        setRole(itr.value().toUInt());
    }
}

Models::Participant::~Participant()
{
}

int Models::Participant::columnCount() const
{
    return 6;
}

QVariant Models::Participant::data(int column) const
{
    switch (column) {
        case 4:
            return static_cast<uint8_t>(affiliation);
        case 5:
            return static_cast<uint8_t>(role);
        default:
            return AbstractParticipant::data(column);
    }
}

void Models::Participant::update(const QString& key, const QVariant& value)
{
    if (key == "affiliation") {
        setAffiliation(value.toUInt());
    } else if (key == "role") {
        setRole(value.toUInt());
    } else {
        AbstractParticipant::update(key, value);
    }
}

Shared::Affiliation Models::Participant::getAffiliation() const
{
    return affiliation;
}

void Models::Participant::setAffiliation(Shared::Affiliation p_aff)
{
    if (p_aff != affiliation) {
        affiliation = p_aff;
        changed(4);
    }
}

void Models::Participant::setAffiliation(unsigned int aff)
{
    if (aff <= static_cast<uint8_t>(Shared::affiliationHighest)) {
        Shared::Affiliation affil = static_cast<Shared::Affiliation>(aff);
        setAffiliation(affil);
    } else {
        qDebug() << "An attempt to set wrong affiliation" << aff << "to the room participant" << name;
    }
}

Shared::Role Models::Participant::getRole() const
{
    return role;
}

void Models::Participant::setRole(Shared::Role p_role)
{
    if (p_role != role) {
        role = p_role;
        changed(5);
    }
}

void Models::Participant::setRole(unsigned int p_role)
{
    if (p_role <= static_cast<uint8_t>(Shared::roleHighest)) {
        Shared::Role r = static_cast<Shared::Role>(p_role);
        setRole(r);
    } else {
        qDebug() << "An attempt to set wrong role" << p_role << "to the room participant" << name;
    }
}
