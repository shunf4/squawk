#include "squawk.h"
#include <QDebug>

Core::Squawk::Squawk(QObject* parent):
    QObject(parent),
    accounts()
{

}

Core::Squawk::~Squawk()
{
    Accounts::const_iterator itr = accounts.begin(); 
    Accounts::const_iterator end = accounts.end();
    for (; itr != end; ++itr) {
        (*itr)->deleteLater();
    }
}

void Core::Squawk::start()
{
}

void Core::Squawk::newAccountRequest(const QMap<QString, QVariant>& map)
{
    QString name = map.value("name").toString();
    QString login = map.value("login").toString();
    QString server = map.value("server").toString();
    QString password = map.value("password").toString();
    
    addAccount(login, server, password, name);
}

void Core::Squawk::addAccount(const QString& login, const QString& server, const QString& password, const QString& name)
{
    Account* acc = new Account(login, server, password, name);
    accounts.push_back(acc);
    
    QMap<QString, QVariant> map = {
        {"login", login},
        {"server", server},
        {"name", name},
        {"password", password},
        {"state", 0}
    };
    emit newAccount(map);
}
