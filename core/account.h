#ifndef CORE_ACCOUNT_H
#define CORE_ACCOUNT_H

#include <QtCore/QObject>

#include <qxmpp/QXmppClient.h>
#include "../global.h"

namespace Core
{

class Account : public QObject
{
    Q_OBJECT
public:
    Account(const QString& p_login, const QString& p_server, const QString& p_password, const QString& p_name, QObject* parent = 0);
    ~Account();
    
    void connect();
    void disconnect();
    
    Shared::ConnectionState getState() const;
    QString getName() const;
    
signals:
    void connectionStateChanged(int);
    
private:
    QString name;
    QString login;
    QString server;
    QString password;
    QXmppClient client;
    Shared::ConnectionState state;
    
private slots:
    void onClientConnected();
    void onClientDisonnected();
    
};

}

#endif // CORE_ACCOUNT_H
