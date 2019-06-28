#ifndef CORE_ACCOUNT_H
#define CORE_ACCOUNT_H

#include <QtCore/QObject>
#include <map>
#include <set>

#include <QXmppRosterManager.h>
#include <QXmppCarbonManager.h>
#include <QXmppMamManager.h>
#include <QXmppClient.h>
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
    void reconnect();
    
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
    void requestArchive(const QString& jid, int count, const QString& before);
    void setReconnectTimes(unsigned int times);
    void subscribeToContact(const QString& jid, const QString& reason);
    void unsubscribeFromContact(const QString& jid, const QString& reason);
    void removeContactRequest(const QString& jid);
    void addContactRequest(const QString& jid, const QString& name, const QSet<QString>& groups);
    
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
    void responseArchive(const QString& jid, const std::list<Shared::Message>& list);
    void error(const QString& text);
    
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
    unsigned int maxReconnectTimes;
    unsigned int reconnectTimes;
    
    std::map<QString, QString> queuedContacts;
    std::set<QString> outOfRosterContacts;
    
private slots:
    void onClientConnected();
    void onClientDisconnected();
    void onClientError(QXmppClient::Error err);
    
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
    
    void onContactGroupAdded(const QString& group);
    void onContactGroupRemoved(const QString& group);
    void onContactNameChanged(const QString& name);
    void onContactSubscriptionStateChanged(Shared::SubscriptionState state);
    void onContactHistoryResponse(const std::list<Shared::Message>& list);
    void onContactNeedHistory(const QString& before, const QString& after);
    
    void onMamLog(QXmppLogger::MessageType type, const QString &msg);
  
private:
    void addedAccount(const QString &bareJid);
    void handleNewContact(Contact* contact);
    bool handleChatMessage(const QXmppMessage& msg, bool outgoing = false, bool forwarded = false, bool guessing = false);
    void addToGroup(const QString& jid, const QString& group);
    void removeFromGroup(const QString& jid, const QString& group);
    void initializeMessage(Shared::Message& target, const QXmppMessage& source, bool outgoing = false, bool forwarded = false, bool guessing = false) const;
    Shared::SubscriptionState castSubscriptionState(QXmppRosterIq::Item::SubscriptionType qs) const;
    void logMessage(const QXmppMessage& msg, const QString& reason = "Message wasn't handled: ");
};

}

#endif // CORE_ACCOUNT_H
