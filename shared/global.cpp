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

#include "global.h"

#include "enums.h"

Shared::Global* Shared::Global::instance = 0;

Shared::Global::Global():
    availability({
        tr("Online"), 
        tr("Away"), 
        tr("Absent"), 
        tr("Busy"), 
        tr("Chatty"), 
        tr("Invisible"), 
        tr("Offline")
    }),
    connectionState({
        tr("Disconnected"), 
        tr("Connecting"), 
        tr("Connected"), 
        tr("Error")
    }),
    subscriptionState({
        tr("None"), 
        tr("From"), 
        tr("To"), 
        tr("Both"), 
        tr("Unknown")
    }),
    affiliation({
        tr("Unspecified"), 
        tr("Outcast"), 
        tr("Nobody"), 
        tr("Member"), 
        tr("Admin"), 
        tr("Owner")
    }),
    role({
        tr("Unspecified"), 
        tr("Nobody"), 
        tr("Visitor"),
        tr("Participant"), 
        tr("Moderator")
    }),
    messageState({
        tr("Pending"), 
        tr("Sent"), 
        tr("Delivered"), 
        tr("Error")
    }),
    accountPassword({
        tr("Plain"),
        tr("Jammed"),
        tr("Always Ask"),
        tr("KWallet")
    })
{
    if (instance != 0) {
        throw 551;
    }
    
    instance = this;
}

Shared::Global * Shared::Global::getInstance()
{
    return instance;
}

QString Shared::Global::getName(Message::State rl)
{
    return instance->messageState[static_cast<int>(rl)];
}

QString Shared::Global::getName(Shared::Affiliation af)
{
    return instance->affiliation[static_cast<int>(af)];
}

QString Shared::Global::getName(Shared::Availability av)
{
    return instance->availability[static_cast<int>(av)];
}

QString Shared::Global::getName(Shared::ConnectionState cs)
{
    return instance->connectionState[static_cast<int>(cs)];
}

QString Shared::Global::getName(Shared::Role rl)
{
    return instance->role[static_cast<int>(rl)];
}

QString Shared::Global::getName(Shared::SubscriptionState ss)
{
    return instance->subscriptionState[static_cast<int>(ss)];
}

QString Shared::Global::getName(Shared::AccountPassword ap)
{
    return instance->accountPassword[static_cast<int>(ap)];
}

#define FROM_INT_INPL(Enum)                                                                 \
template<>                                                                                  \
Enum Shared::Global::fromInt(int src)                                                       \
{                                                                                           \
    if (src < static_cast<int>(Enum##Lowest) && src > static_cast<int>(Enum##Highest)) {    \
        throw EnumOutOfRange(#Enum);                                                        \
    }                                                                                       \
    return static_cast<Enum>(src);                                                          \
}                                                                                           \
template<>                                                                                  \
Enum Shared::Global::fromInt(unsigned int src) {return fromInt<Enum>(static_cast<int>(src));}

FROM_INT_INPL(Shared::Message::State)
FROM_INT_INPL(Shared::Affiliation)
FROM_INT_INPL(Shared::ConnectionState)
FROM_INT_INPL(Shared::Role)
FROM_INT_INPL(Shared::SubscriptionState)
FROM_INT_INPL(Shared::AccountPassword)
FROM_INT_INPL(Shared::Avatar)
FROM_INT_INPL(Shared::Availability)
