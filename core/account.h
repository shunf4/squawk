#ifndef CORE_ACCOUNT_H
#define CORE_ACCOUNT_H

#include <QtCore/QObject>
#include <map>

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
    QString getLogin() const;
    QString getServer() const;
    QString getPassword() const;
    
signals:
    void connectionStateChanged(int);
    void addGroup(const QString& name);
    void removeGroup(const QString& name);
    void addContact(const QString& jid, const QString& name, const QString& group);
    
private:
    QString name;
    QString login;
    QString server;
    QString password;
    QXmppClient client;
    Shared::ConnectionState state;
    std::map<QString, int> groups;
    
private slots:
    void onClientConnected();
    void onClientDisconnected();
    void onRosterReceived();
    
};

}

#endif // CORE_ACCOUNT_H
