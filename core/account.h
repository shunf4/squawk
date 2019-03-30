#ifndef CORE_ACCOUNT_H
#define CORE_ACCOUNT_H

#include <QtCore/QObject>

#include <qxmpp/QXmppClient.h>

namespace Core
{

class Account : public QObject
{
public:
    Account(const QString& p_login, const QString& p_server, const QString& p_password, const QString& p_name, QObject* parent = 0);
    ~Account();
    
private:
    QString name;
    QString login;
    QString server;
    QString password;
    QXmppClient client;
};

}

#endif // CORE_ACCOUNT_H
