#include "account.h"

using namespace Core;

Account::Account(const QString& p_jid, const QString& p_password, QObject* parent):
    QObject(parent),
    jid(p_jid),
    password(p_password),
    client()
{

}

Account::~Account()
{

}
