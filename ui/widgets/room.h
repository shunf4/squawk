/*
 * Squawk messenger.
 * Copyright (C) 2019 Yury Gubich <blue@macaw.me>
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

#ifndef ROOM_H
#define ROOM_H

#include "conversation.h"
#include "../models/room.h"

/**
 * @todo write docs
 */
class Room : public Conversation
{
    Q_OBJECT
public:
    Room(Models::Account* acc, Models::Room* p_room, QWidget* parent = 0);
    ~Room();
    
    bool autoJoined() const;
    
protected slots:
    void onRoomChanged(Models::Item* item, int row, int col);
    void onParticipantJoined(const Models::Participant& participant);
    void onParticipantLeft(const QString& name);
    
protected:
    Shared::Message createMessage() const override;
    
private:
    Models::Room* room;
    
};

#endif // ROOM_H
