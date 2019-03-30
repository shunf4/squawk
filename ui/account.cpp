#include "account.h"
#include "ui_account.h"

Account::Account()
    : m_ui ( new Ui::Account )
{
    m_ui->setupUi ( this );
}

Account::~Account()
{
}

QMap<QString, QVariant> Account::value() const
{
    QMap<QString, QVariant> map;
    map["login"] = m_ui->login->text();
    map["password"] = m_ui->password->text();
    map["server"] = m_ui->server->text();
    map["name"] = m_ui->name->text();
    
    return map;
}
