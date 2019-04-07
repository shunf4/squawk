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
    void accountConnectionStateChanged(const QString&, int);
    void addGroup(const QString& account, const QString& name);
    void removeGroup(const QString& account, const QString& name);
    void addContact(const QString& account, const QString& jid, const QString& name, const QString& group);
    void removeContact(const QString& account, const QString& jid);
    void removeContact(const QString& account, const QString& jid, const QString& group);
    void changeContact(const QString& account, const QString& jid, const QString& name);
    void addPresence(const QString& account, const QString& jid, const QString& name, const QMap<QString, QVariant>& data);
    void removePresence(const QString& account, const QString& jid, const QString& name);
    
public slots:
    void start();
    void stop();
    void newAccountRequest(const QMap<QString, QVariant>& map);
    void connectAccount(const QString& account);
    void disconnectAccount(const QString& account);
    
private:
    typedef std::deque<Account*> Accounts;
    typedef std::map<QString, Account*> AccountsMap;
    
    Accounts accounts;
    AccountsMap amap;
    
private:
    void addAccount(const QString& login, const QString& server, const QString& password, const QString& name);
    
private slots:
    void onAccountConnectionStateChanged(int state);
    void onAccountAddGroup(const QString& name);
    void onAccountRemoveGroup(const QString& name);
    void onAccountAddContact(const QString& jid, const QString& name, const QString& group);
    void onAccountRemoveContact(const QString& jid);
    void onAccountRemoveContact(const QString& jid, const QString& group);
    void onAccountChangeContact(const QString& jid, const QString& name);
    void onAccountAddPresence(const QString& jid, const QString& name, const QMap<QString, QVariant>& data);
    void onAccountRemovePresence(const QString& jid, const QString& name);
};

}

#endif // CORE_SQUAWK_H
