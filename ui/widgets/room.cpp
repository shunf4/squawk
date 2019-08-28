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

Room::Room(Models::Room* p_room, QWidget* parent):
    Conversation(p_room->getAccountJid(), p_room->getAccountResource(), p_room->getJid(), "", p_room->getAccountName(), parent),
    room(p_room)
{
    setName(p_room->getName());
    line->setMyName(room->getNick());
    line->setRoom(true);
}

Room::~Room()
{
}

void Room::handleSendMessage(const QString& text)
{
    Shared::Message msg(Shared::Message::groupChat);
    msg.setFromJid(myJid);
    msg.setFromResource(myResource);
    msg.setToJid(palJid);
    //msg.setToResource(activePalResource);
    msg.setBody(text);
    msg.setOutgoing(true);
    msg.generateRandomId();
    msg.setCurrentTime();
    emit sendMessage(msg);
}
