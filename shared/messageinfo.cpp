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

#include "messageinfo.h"

using namespace Shared;

Shared::MessageInfo::MessageInfo():
    account(),
    jid(),
    messageId() {}

Shared::MessageInfo::MessageInfo(const QString& acc, const QString& j, const QString& id):
    account(acc),
    jid(j),
    messageId(id) {}

Shared::MessageInfo::MessageInfo(const Shared::MessageInfo& other):
    account(other.account),
    jid(other.jid),
    messageId(other.messageId) {}

Shared::MessageInfo & Shared::MessageInfo::operator=(const Shared::MessageInfo& other)
{
    account = other.account;
    jid = other.jid;
    messageId = other.messageId;
    
    return *this;
}
