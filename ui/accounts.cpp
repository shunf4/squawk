#include "accounts.h"
#include "ui_accounts.h"

Accounts::Accounts(QWidget *parent) :
    m_ui(new Ui::Accounts)
{
    m_ui->setupUi(this);
}

Accounts::~Accounts() = default;
