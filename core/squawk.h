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
};

}

#endif // CORE_SQUAWK_H
