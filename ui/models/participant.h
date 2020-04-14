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

#ifndef MODELS_PARTICIPANT_H
#define MODELS_PARTICIPANT_H

#include "abstractparticipant.h"
#include "shared/global.h"

namespace Models {

class Participant : public Models::AbstractParticipant
{
public:
    Participant(const QMap<QString, QVariant> &data, Item *parentItem = 0);
    ~Participant();
    
    int columnCount() const override;
    QVariant data(int column) const override;

    void update(const QString& key, const QVariant& value) override;
    
    Shared::Affiliation getAffiliation() const;
    void setAffiliation(Shared::Affiliation p_aff);
    void setAffiliation(unsigned int aff);
    
    Shared::Role getRole() const;
    void setRole(Shared::Role p_role);
    void setRole(unsigned int role);
    
    Shared::Avatar getAvatarState() const;
    QString getAvatarPath() const;

protected:
    void setAvatarState(Shared::Avatar p_state);
    void setAvatarState(unsigned int p_state);
    void setAvatarPath(const QString& path);
    
private:
    Shared::Affiliation affiliation;
    Shared::Role role;
    Shared::Avatar avatarState;
    QString avatarPath;
};

}

#endif // MODELS_PARTICIPANT_H
