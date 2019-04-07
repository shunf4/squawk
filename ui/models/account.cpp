#include "account.h"
#include <QDebug>

Models::Account::Account(const QMap<QString, QVariant>& data, Models::Item* parentItem):
    Item(account, data, parentItem),
    login(data.value("login").toString()),
    password(data.value("password").toString()),
    server(data.value("server").toString()),
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
        changed(5);
    }
}

QIcon Models::Account::getStatusIcon() const
{
    if (state == Shared::connected) {
        return QIcon::fromTheme(Shared::availabilityThemeIcons[availability]);
    } else if (state == Shared::disconnected) {
        return QIcon::fromTheme(Shared::availabilityThemeIcons[Shared::offline]);
    } else {
        return QIcon::fromTheme(Shared::connectionStateThemeIcons[state]);
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
        changed(3);
    }
}

void Models::Account::setPassword(const QString& p_password)
{
    if (password != p_password) {
        password = p_password;
        changed(4);
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
            return Shared::connectionStateNames[state];
        case 3:
            return login;
        case 4:
            return password;
        case 5:
            return Shared::availabilityNames[availability];
        default:
            return QVariant();
    }
}

int Models::Account::columnCount() const
{
    return 6;
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
    }
}
