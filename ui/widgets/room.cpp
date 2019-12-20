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

#include "room.h"

Room::Room(Models::Account* acc, Models::Room* p_room, QWidget* parent):
    Conversation(true, acc, p_room->getJid(), "", parent),
    room(p_room)
{
    setName(p_room->getName());
    line->setMyName(room->getNick());
    setStatus(room->getSubject());
    
    connect(room, &Models::Room::childChanged, this, &Room::onRoomChanged);
}

Room::~Room()
{
}

void Room::handleSendMessage(const QString& text)
{
    Shared::Message msg(Shared::Message::groupChat);
    msg.setFrom(account->getFullJid());
    msg.setToJid(palJid);
    //msg.setToResource(activePalResource);
    msg.setBody(text);
    msg.setOutgoing(true);
    msg.generateRandomId();
    msg.setCurrentTime();
    emit sendMessage(msg);
}

bool Room::autoJoined() const
{
    return room->getAutoJoin();
}

void Room::onRoomChanged(Models::Item* item, int row, int col)
{
    if (item == room) {
        switch (col) {
            case 0:
                setName(room->getRoomName());
                break;
            case 6:
                setStatus(room->getSubject());
                break;
        }
    }
}
