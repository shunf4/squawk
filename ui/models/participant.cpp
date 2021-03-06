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
    
    itr = data.find("avatarState");
    if (itr != data.end()) {
        setAvatarState(itr.value().toUInt());
    }
    itr = data.find("avatarPath");
    if (itr != data.end()) {
        setAvatarPath(itr.value().toString());
    }
}

Models::Participant::~Participant()
{
}

int Models::Participant::columnCount() const
{
    return 8;
}

QVariant Models::Participant::data(int column) const
{
    switch (column) {
        case 4:
            return QVariant::fromValue(affiliation);
        case 5:
            return QVariant::fromValue(role);
        case 6:
            return QVariant::fromValue(getAvatarState());
        case 7:
            return getAvatarPath();
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
    } else if (key == "avatarState") {
        setAvatarState(value.toUInt());
    } else if (key == "avatarPath") {
        setAvatarPath(value.toString());
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
    setAffiliation(Shared::Global::fromInt<Shared::Affiliation>(aff));
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
    setRole(Shared::Global::fromInt<Shared::Role>(p_role));
}

QString Models::Participant::getAvatarPath() const
{
    return avatarPath;
}

Shared::Avatar Models::Participant::getAvatarState() const
{
    return avatarState;
}

void Models::Participant::setAvatarPath(const QString& path)
{
    if (avatarPath != path) {
        avatarPath = path;
        changed(7);
    }
}

void Models::Participant::setAvatarState(Shared::Avatar p_state)
{
    if (avatarState != p_state) {
        avatarState = p_state;
        changed(6);
    }
}

void Models::Participant::setAvatarState(unsigned int p_state)
{setAvatarState(Shared::Global::fromInt<Shared::Avatar>(p_state));}
