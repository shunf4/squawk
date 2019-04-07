#ifndef CORE_ACCOUNT_H
#define CORE_ACCOUNT_H

#include <QtCore/QObject>
#include <map>
#include <set>

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
    void removeContact(const QString& jid);
    void removeContact(const QString& jid, const QString& group);
    void changeContact(const QString& jid, const QString& name);
    void addPresence(const QString& jid, const QString& name, const QMap<QString, QVariant>& data);
    void removePresence(const QString& jid, const QString& name);
    
private:
    QString name;
    QString login;
    QString server;
    QString password;
    QXmppClient client;
    Shared::ConnectionState state;
    std::map<QString, std::set<QString>> groups;
    
private slots:
    void onClientConnected();
    void onClientDisconnected();
    void onRosterReceived();
    void onRosterItemAdded(const QString& bareJid);
    void onRosterItemChanged(const QString& bareJid);
    void onRosterItemRemoved(const QString& bareJid);
    void onRosterPresenceChanged(const QString& bareJid, const QString& resource);
    void onPresenceReceived(const QXmppPresence& presence);
  
private:
    void addedAccount(const QString &bareJid);
};

}

#endif // CORE_ACCOUNT_H
