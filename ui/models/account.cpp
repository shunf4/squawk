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
    state = p_state;
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
    login = p_login;
}

void Models::Account::setPassword(const QString& p_password)
{
    password = p_password;
}

void Models::Account::setServer(const QString& p_server)
{
    server = p_server;
}

QVariant Models::Account::data(int column) const
{
    switch (column) {
        case 0:
            return Item::data(column);
        case 1:
            return server;
        case 2:
            return state;
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
