#include "account.h"

using namespace Core;

Account::Account(const QString& p_login, const QString& p_server, const QString& p_password, const QString& p_name, QObject* parent):
    QObject(parent),
    name(p_name),
    login(p_login),
    server(p_server),
    password(p_password),
    client()
{

}

Account::~Account()
{

}
