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
    setAvatar(room->getAvatarPath());
    
    connect(room, &Models::Room::childChanged, this, &Room::onRoomChanged);
    connect(room, &Models::Room::participantJoined, this, &Room::onParticipantJoined);
    connect(room, &Models::Room::participantLeft, this, &Room::onParticipantLeft);
    
    std::map<QString, const Models::Participant&> members = room->getParticipants();
    for (std::pair<QString, const Models::Participant&> pair : members) {
        QString aPath = pair.second.getAvatarPath();
        if (aPath.size() > 0) {
            line->setPalAvatar(pair.first, aPath);
        }
    }
    
    line->setExPalAvatars(room->getExParticipantAvatars());
}

Room::~Room()
{
}

Shared::Message Room::createMessage() const
{
    Shared::Message msg = Conversation::createMessage();
    msg.setType(Shared::Message::groupChat);
    msg.setFromJid(room->getJid());
    msg.setFromResource(room->getNick());
    msg.setToJid(palJid);
    return msg;
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
            case 8:
                setAvatar(room->getAvatarPath());
                break;
        }
    } else {
        switch (col) {
            case 7: {
                Models::Participant* mem = static_cast<Models::Participant*>(item);
                QString aPath = mem->getAvatarPath();
                if (aPath.size() > 0) {
                    line->setPalAvatar(mem->getName(), aPath);
                } else {
                    line->dropPalAvatar(mem->getName());
                }
            }
        }
    }
}

void Room::onParticipantJoined(const Models::Participant& participant)
{
    QString aPath = participant.getAvatarPath();
    if (aPath.size() > 0) {
        line->setPalAvatar(participant.getName(), aPath);
    }
}

void Room::onParticipantLeft(const QString& name)
{
    line->movePalAvatarToEx(name);
}
