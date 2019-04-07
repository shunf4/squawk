#include "account.h"

Models::Account::Account(const QMap<QString, QVariant>& data, Models::Item* parentItem):
    Item(account, data, parentItem),
    login(data.value("login").toString()),
    password(data.value("password").toString()),
    server(data.value("server").toString()),
    state(data.value("state").toInt())
{
}

Models::Account::~Account()
{
}

void Models::Account::setState(int p_state)
{
    if (state != p_state) {
        state = p_state;
        changed(2);
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

int Models::Account::getState() const
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
            return Shared::ConnectionStateNames[state];
        case 3:
            return login;
        case 4:
            return password;
        default:
            return QVariant();
    }
}

int Models::Account::columnCount() const
{
    return 5;
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
        setState(value.toInt());
    }
}
