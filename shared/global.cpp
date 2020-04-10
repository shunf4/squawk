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
        tr("Online", "Availability"), 
        tr("Away", "Availability"), 
        tr("Absent", "Availability"), 
        tr("Busy", "Availability"), 
        tr("Chatty", "Availability"), 
        tr("Invisible", "Availability"), 
        tr("Offline", "Availability")
    }),
    connectionState({
        tr("Disconnected", "ConnectionState"), 
        tr("Connecting", "ConnectionState"), 
        tr("Connected", "ConnectionState"), 
        tr("Error", "ConnectionState")
    }),
    subscriptionState({
        tr("None", "SubscriptionState"), 
        tr("From", "SubscriptionState"), 
        tr("To", "SubscriptionState"), 
        tr("Both", "SubscriptionState"), 
        tr("Unknown", "SubscriptionState")
    }),
    affiliation({
        tr("Unspecified", "Affiliation"), 
        tr("Outcast", "Affiliation"), 
        tr("Nobody", "Affiliation"), 
        tr("Member", "Affiliation"), 
        tr("Admin", "Affiliation"), 
        tr("Owner", "Affiliation")
    }),
    role({
        tr("Unspecified", "Role"), 
        tr("Nobody", "Role"), 
        tr("Visitor", "Role"),
        tr("Participant", "Role"), 
        tr("Moderator", "Role")
    }),
    messageState({
        tr("Pending", "MessageState"), 
        tr("Sent", "MessageState"), 
        tr("Delivered", "MessageState"), 
        tr("Error", "MessageState")
    }),
    accountPassword({
        tr("Plain", "AccountPassword"),
        tr("Jammed", "AccountPassword"),
        tr("Always Ask", "AccountPassword"),
        tr("KWallet", "AccountPassword")
    }),
    accountPasswordDescription({
        tr("Your password is going to be stored in config file in plain text", "AccountPasswordDescription"),
        tr("Your password is going to be stored in config file but jammed with constant encryption key you can find in program source code. It might look like encryption but it's not", "AccountPasswordDescription"),
        tr("Squawk is going to query you for the password on every start of the program", "AccountPasswordDescription"),
        tr("Your password is going to be stored in KDE wallet storage (KWallet). You're going to be queried for permissions", "AccountPasswordDescription")
    }),
    pluginSupport({
        {"KWallet", false}
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

void Shared::Global::setSupported(const QString& pluginName, bool support)
{
    std::map<QString, bool>::iterator itr = instance->pluginSupport.find(pluginName);
    if (itr != instance->pluginSupport.end()) {
        itr->second = support;
    }
}

bool Shared::Global::supported(const QString& pluginName)
{
    std::map<QString, bool>::iterator itr = instance->pluginSupport.find(pluginName);
    if (itr != instance->pluginSupport.end()) {
        return itr->second;
    }
    return false;
}

QString Shared::Global::getDescription(Shared::AccountPassword ap)
{
    return instance->accountPasswordDescription[static_cast<int>(ap)];
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
