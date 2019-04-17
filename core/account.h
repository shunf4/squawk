#ifndef CORE_ACCOUNT_H
#define CORE_ACCOUNT_H

#include <QtCore/QObject>
#include <map>
#include <set>

#include <qxmpp/QXmppRosterManager.h>
#include <qxmpp/QXmppCarbonManager.h>
#include <qxmpp/QXmppMamManager.h>
#include <qxmpp/QXmppClient.h>
#include "../global.h"
#include "contact.h"

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
    QString getResource() const;
    Shared::Availability getAvailability() const;
    
    void setName(const QString& p_name);
    void setLogin(const QString& p_login);
    void setServer(const QString& p_server);
    void setPassword(const QString& p_password);
    void setResource(const QString& p_resource);
    void setAvailability(Shared::Availability avail);
    QString getFullJid() const;
    void sendMessage(const Shared::Message& data);
    void requestAchive(const QString& jid);
    
signals:
    void connectionStateChanged(int);
    void availabilityChanged(int);
    void addGroup(const QString& name);
    void removeGroup(const QString& name);
    void addContact(const QString& jid, const QString& group, const QMap<QString, QVariant>& data);
    void removeContact(const QString& jid);
    void removeContact(const QString& jid, const QString& group);
    void changeContact(const QString& jid, const QMap<QString, QVariant>& data);
    void addPresence(const QString& jid, const QString& name, const QMap<QString, QVariant>& data);
    void removePresence(const QString& jid, const QString& name);
    void message(const Shared::Message& data);
    
private:
    QString name;
    std::map<QString, QString> achiveQueries;
    QXmppClient client;
    QXmppConfiguration config;
    QXmppPresence presence;
    Shared::ConnectionState state;
    std::map<QString, std::set<QString>> groups;
    QXmppCarbonManager* cm;
    QXmppMamManager* am;
    std::map<QString, Contact*> contacts; 
    
private slots:
    void onClientConnected();
    void onClientDisconnected();
    void onRosterReceived();
    void onRosterItemAdded(const QString& bareJid);
    void onRosterItemChanged(const QString& bareJid);
    void onRosterItemRemoved(const QString& bareJid);
    void onRosterPresenceChanged(const QString& bareJid, const QString& resource);
    void onPresenceReceived(const QXmppPresence& presence);
    void onMessageReceived(const QXmppMessage& message);
    void onCarbonMessageReceived(const QXmppMessage& message);
    void onCarbonMessageSent(const QXmppMessage& message);
    void onMamMessageReceived(const QString& bareJid, const QXmppMessage& message);
    void onMamResultsReceived(const QString &queryId, const QXmppResultSetReply &resultSetReply, bool complete);
  
private:
    void addedAccount(const QString &bareJid);
    bool handleChatMessage(const QXmppMessage& msg, bool outgoing = false, bool forwarded = false, bool guessing = false);
};

}

#endif // CORE_ACCOUNT_H
