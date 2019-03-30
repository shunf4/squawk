#ifndef CORE_SQUAWK_H
#define CORE_SQUAWK_H

#include <QtCore/QObject>
#include <QString>
#include <QVariant>
#include <QMap>
#include <deque>

#include "account.h"

namespace Core
{
class Squawk : public QObject
{
    Q_OBJECT

public:
    Squawk(QObject* parent = 0);
    ~Squawk();

signals:
    void newAccount(const QMap<QString, QVariant>&);
    
public slots:
    void start();
    void newAccountRequest(const QMap<QString, QVariant>& map);
    
private:
    typedef std::deque<Account*> Accounts;
    
    Accounts accounts;
    
private:
    void addAccount(const QString& login, const QString& server, const QString& password, const QString& name);
};

}

#endif // CORE_SQUAWK_H
