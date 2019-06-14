#ifndef CORE_SQUAWK_H
#define CORE_SQUAWK_H

#include <QtCore/QObject>
#include <QString>
#include <QVariant>
#include <QMap>
#include <deque>
#include <deque>

#include "account.h"
#include "../global.h"

namespace Core
{
class Squawk : public QObject
{
    Q_OBJECT

public:
    Squawk(QObject* parent = 0);
    ~Squawk();

signals:
    void quit();
    void newAccount(const QMap<QString, QVariant>&);
    void changeAccount(const QString& account, const QMap<QString, QVariant>& data);
    void removeAccount(const QString& account);
    void addGroup(const QString& account, const QString& name);
    void removeGroup(const QString& account, const QString& name);
    void addContact(const QString& account, const QString& jid, const QString& group, const QMap<QString, QVariant>& data);
    void removeContact(const QString& account, const QString& jid);
    void removeContact(const QString& account, const QString& jid, const QString& group);
    void changeContact(const QString& account, const QString& jid, const QMap<QString, QVariant>& data);
    void addPresence(const QString& account, const QString& jid, const QString& name, const QMap<QString, QVariant>& data);
    void removePresence(const QString& account, const QString& jid, const QString& name);
    void stateChanged(int state);
    void accountMessage(const QString& account, const Shared::Message& data);
    void responseArchive(const QString& account, const QString& jid, const std::list<Shared::Message>& list);
    
public slots:
    void start();
    void stop();
    void newAccountRequest(const QMap<QString, QVariant>& map);
    void modifyAccountRequest(const QString& name, const QMap<QString, QVariant>& map);
    void removeAccountRequest(const QString& name);
    void connectAccount(const QString& account);
    void disconnectAccount(const QString& account);
    void changeState(int state);
    void sendMessage(const QString& account, const Shared::Message& data);
    void requestArchive(const QString& account, const QString& jid, int count, const QString& before);
    void subscribeContact(const QString& account, const QString& jid, const QString& reason);
    void unsubscribeContact(const QString& account, const QString& jid, const QString& reason);
    void removeContactRequest(const QString& account, const QString& jid);
    
private:
    typedef std::deque<Account*> Accounts;
    typedef std::map<QString, Account*> AccountsMap;
    
    Accounts accounts;
    AccountsMap amap;
    Shared::Availability state;
    
private:
    void addAccount(const QString& login, const QString& server, const QString& password, const QString& name, const QString& resource);
    
private slots:
    void onAccountConnectionStateChanged(int state);
    void onAccountAvailabilityChanged(int state);
    void onAccountAddGroup(const QString& name);
    void onAccountError(const QString& text);
    void onAccountRemoveGroup(const QString& name);
    void onAccountAddContact(const QString& jid, const QString& group, const QMap<QString, QVariant>& data);
    void onAccountRemoveContact(const QString& jid);
    void onAccountRemoveContact(const QString& jid, const QString& group);
    void onAccountChangeContact(const QString& jid, const QMap<QString, QVariant>& data);
    void onAccountAddPresence(const QString& jid, const QString& name, const QMap<QString, QVariant>& data);
    void onAccountRemovePresence(const QString& jid, const QString& name);
    void onAccountMessage(const Shared::Message& data);
    void onAccountResponseArchive(const QString& jid, const std::list<Shared::Message>& list);
};

}

#endif // CORE_SQUAWK_H
