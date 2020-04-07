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

#ifndef SHARED_ENUMS_H
#define SHARED_ENUMS_H

#include <deque>

#include <QString>
#include <QObject>

namespace Shared {
Q_NAMESPACE
    
enum class ConnectionState {
    disconnected,
    connecting,
    connected,
    error
};
Q_ENUM_NS(ConnectionState)
static const std::deque<QString> connectionStateThemeIcons = {"state-offline", "state-sync", "state-ok", "state-error"};
static const ConnectionState ConnectionStateHighest = ConnectionState::error;
static const ConnectionState ConnectionStateLowest = ConnectionState::disconnected;

enum class Availability {
    online,
    away,
    extendedAway,
    busy,
    chatty,
    invisible,
    offline
};
Q_ENUM_NS(Availability)
static const Availability AvailabilityHighest = Availability::offline;
static const Availability AvailabilityLowest = Availability::online;
static const std::deque<QString> availabilityThemeIcons = {
    "user-online",
    "user-away",
    "user-away-extended",
    "user-busy",
    "chatty",
    "user-invisible",
    "user-offline"
};

enum class SubscriptionState {
    none,
    from,
    to,
    both,
    unknown
};
Q_ENUM_NS(SubscriptionState)
static const SubscriptionState SubscriptionStateHighest = SubscriptionState::unknown;
static const SubscriptionState SubscriptionStateLowest = SubscriptionState::none;
static const std::deque<QString> subscriptionStateThemeIcons = {"edit-none", "arrow-down-double", "arrow-up-double", "dialog-ok", "question"};

enum class Affiliation {
    unspecified, 
    outcast, 
    nobody, 
    member, 
    admin, 
    owner 
};
Q_ENUM_NS(Affiliation)
static const Affiliation AffiliationHighest = Affiliation::owner;
static const Affiliation AffiliationLowest = Affiliation::unspecified;

enum class Role { 
    unspecified, 
    nobody, 
    visitor, 
    participant, 
    moderator 
};
Q_ENUM_NS(Role)
static const Role RoleHighest = Role::moderator;
static const Role RoleLowest = Role::unspecified;

enum class Avatar {
    empty,
    autocreated,
    valid
};
Q_ENUM_NS(Avatar)
static const Avatar AvatarHighest = Avatar::valid; 
static const Avatar AvatarLowest = Avatar::empty; 


static const std::deque<QString> messageStateThemeIcons = {"state-offline", "state-sync", "state-ok", "state-error"};

enum class AccountPassword {
    plain,
    jammed,
    alwaysAsk,
    kwallet
};
Q_ENUM_NS(AccountPassword)
static const AccountPassword AccountPasswordHighest = AccountPassword::kwallet;
static const AccountPassword AccountPasswordLowest = AccountPassword::plain;

}
#endif // SHARED_ENUMS_H
