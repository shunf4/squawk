#include "accounts.h"
#include "ui_accounts.h"

#include <QDebug>

Accounts::Accounts(QWidget *parent) :
    m_ui(new Ui::Accounts),
    tableModel()
{
    m_ui->setupUi(this);
    
    connect(m_ui->addButton, SIGNAL(clicked(bool)), this, SLOT(onAddButton(bool)));
    m_ui->tableView->setModel(&tableModel);
}

Accounts::~Accounts() = default;

void Accounts::onAddButton(bool clicked)
{
    Account* acc = new Account();
    connect(acc, SIGNAL(accepted()), this, SLOT(onAccountAccepted()));
    connect(acc, SIGNAL(rejected()), this, SLOT(onAccountRejected()));
    acc->exec();
}

void Accounts::onAccountAccepted()
{
    Account* acc = static_cast<Account*>(sender());
    QMap<QString, QVariant> map = acc->value();
    
    emit newAccount(map);
}

void Accounts::onAccountRejected()
{
    Account* acc = static_cast<Account*>(sender());
    acc->deleteLater();
}

void Accounts::addAccount(const QMap<QString, QVariant>& map)
{
    tableModel.addAccount(map);
}
