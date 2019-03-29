#ifndef CORE_ACCOUNT_H
#define CORE_ACCOUNT_H

#include <QtCore/QObject>

#include <qxmpp/QXmppClient.h>

namespace Core
{

class Account : public QObject
{
public:
    Account(const QString& p_jid, const QString& p_password, QObject* parent = 0);
    ~Account();
    
private:
    QString jid;
    QString password;
    QXmppClient client;
};

}

#endif // CORE_ACCOUNT_H
