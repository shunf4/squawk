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

#include "account.h"
#include <QDebug>

Models::Account::Account(const QMap<QString, QVariant>& data, Models::Item* parentItem):
    Item(account, data, parentItem),
    login(data.value("login").toString()),
    password(data.value("password").toString()),
    server(data.value("server").toString()),
    resource(data.value("resource").toString()),
    error(data.value("error").toString()),
    state(Shared::disconnected),
    availability(Shared::offline)
{
    QMap<QString, QVariant>::const_iterator sItr = data.find("state");
    if (sItr != data.end()) {
        setState(sItr.value().toUInt());
    }
    QMap<QString, QVariant>::const_iterator aItr = data.find("availability");
    if (aItr != data.end()) {
        setAvailability(aItr.value().toUInt());
    }
}

Models::Account::~Account()
{
}

void Models::Account::setState(Shared::ConnectionState p_state)
{
    if (state != p_state) {
        state = p_state;
        changed(2);
        if (state == Shared::disconnected) {
            toOfflineState();
        }
    }
}

void Models::Account::setAvailability(unsigned int p_state)
{
    if (p_state <= Shared::availabilityHighest) {
        Shared::Availability state = static_cast<Shared::Availability>(p_state);
        setAvailability(state);
    } else {
        qDebug() << "An attempt to set invalid availability " << p_state << " to the account " << name;
    }
}

void Models::Account::setState(unsigned int p_state)
{
    if (p_state <= Shared::subscriptionStateHighest) {
        Shared::ConnectionState state = static_cast<Shared::ConnectionState>(p_state);
        setState(state);
    } else {
        qDebug() << "An attempt to set invalid subscription state " << p_state << " to the account " << name;
    }
}

Shared::Availability Models::Account::getAvailability() const
{
    return availability;
}

void Models::Account::setAvailability(Shared::Availability p_avail)
{
    if (availability != p_avail) {
        availability = p_avail;
        changed(6);
    }
}

QIcon Models::Account::getStatusIcon(bool big) const
{
    if (state == Shared::connected) {
        return Shared::availabilityIcon(availability, big);
    } else if (state == Shared::disconnected) {
        return Shared::availabilityIcon(Shared::offline, big);
    } else {
        return Shared::connectionStateIcon(state);
    }
}


QString Models::Account::getLogin() const
{
    return login;
}

QString Models::Account::getPassword() const
{
    return password;
}

QString Models::Account::getServer() const
{
    return server;
}

Shared::ConnectionState Models::Account::getState() const
{
    return state;
}

void Models::Account::setLogin(const QString& p_login)
{
    if (login != p_login) {
        login = p_login;
        changed(4);
    }
}

void Models::Account::setPassword(const QString& p_password)
{
    if (password != p_password) {
        password = p_password;
        changed(5);
    }
}

void Models::Account::setServer(const QString& p_server)
{
    if (server != p_server) {
        server = p_server;
        changed(1);
    }
}

QVariant Models::Account::data(int column) const
{
    switch (column) {
        case 0:
            return Item::data(column);
        case 1:
            return server;
        case 2:
            return QCoreApplication::translate("Global", Shared::connectionStateNames[state].toLatin1());
        case 3:
            return error;
        case 4:
            return login;
        case 5:
            return password;
        case 6:
            return QCoreApplication::translate("Global", Shared::availabilityNames[availability].toLatin1());
        case 7:
            return resource;
        default:
            return QVariant();
    }
}

int Models::Account::columnCount() const
{
    return 8;
}

void Models::Account::update(const QString& field, const QVariant& value)
{
    if (field == "name") {
        setName(value.toString());
    } else if (field == "server") {
        setServer(value.toString());
    } else if (field == "login") {
        setLogin(value.toString());
    } else if (field == "password") {
        setPassword(value.toString());
    } else if (field == "state") {
        setState(value.toUInt());
    } else if (field == "availability") {
        setAvailability(value.toUInt());
    } else if (field == "resource") {
        setResource(value.toString());
    } else if (field == "error") {
        setError(value.toString());
    }
}

QString Models::Account::getResource() const
{
    return resource;
}

void Models::Account::setResource(const QString& p_resource)
{
    if (resource != p_resource) {
        resource = p_resource;
        changed(7);
    }
}

QString Models::Account::getError() const
{
    return error;
}

void Models::Account::setError(const QString& p_resource)
{
    if (error != p_resource) {
        error = p_resource;
        changed(3);
    }
}

void Models::Account::toOfflineState()
{
    setAvailability(Shared::offline);
    Item::toOfflineState();
}
