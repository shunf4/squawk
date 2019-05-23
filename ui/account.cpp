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
    map["resource"] = m_ui->resource->text();
    
    return map;
}

void Account::lockId()
{
    m_ui->name->setReadOnly(true);;
}

void Account::setData(const QMap<QString, QVariant>& data)
{
    m_ui->login->setText(data.value("login").toString());
    m_ui->password->setText(data.value("password").toString());
    m_ui->server->setText(data.value("server").toString());
    m_ui->name->setText(data.value("name").toString());
    m_ui->resource->setText(data.value("resource").toString());
}
