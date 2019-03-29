#ifndef CORE_SQUAWK_H
#define CORE_SQUAWK_H

#include <QtCore/QObject>
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

private:
    typedef std::deque<Account> Accounts;
    
    Accounts accounts;
};

}

#endif // CORE_SQUAWK_H
