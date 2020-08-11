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

#ifndef MODELS_PRESENCE_H
#define MODELS_PRESENCE_H

#include "abstractparticipant.h"
#include "shared/enums.h"
#include "shared/message.h"

#include <QDateTime>
#include <QIcon>

namespace Models {

class Presence : public Models::AbstractParticipant
{
    Q_OBJECT
public:
    explicit Presence(const QMap<QString, QVariant> &data, Item *parentItem = 0);
    ~Presence();
    
    int columnCount() const override;
};

}

#endif // MODELS_PRESENCE_H
