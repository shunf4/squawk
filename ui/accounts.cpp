#include "accounts.h"
#include "ui_accounts.h"

#include <QDebug>

Accounts::Accounts(Models::Accounts* p_model, QWidget *parent) :
    m_ui(new Ui::Accounts),
    model(p_model),
    editing(false)
{
    m_ui->setupUi(this);
    
    connect(m_ui->addButton, SIGNAL(clicked(bool)), this, SLOT(onAddButton(bool)));
    connect(m_ui->editButton, SIGNAL(clicked(bool)), this, SLOT(onEditButton(bool)));
    m_ui->tableView->setModel(model);
    connect(m_ui->tableView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
            this, SLOT(onSelectionChanged(const QItemSelection&, const QItemSelection&)));
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
    if (editing) {
        emit changeAccount(map);
    } else {
        emit newAccount(map);
    }
    
    editing = false;
}

void Accounts::onAccountRejected()
{
    Account* acc = static_cast<Account*>(sender());
    acc->deleteLater();
    editing = false;
}

void Accounts::onEditButton(bool clicked)
{
    Account* acc = new Account();
    
    const Models::Account* mAcc = model->getAccount(m_ui->tableView->selectionModel()->selectedRows().at(0).row());
    acc->setData({
        {"login", mAcc->getLogin()},
        {"password", mAcc->getPassword()},
        {"server", mAcc->getServer()},
        {"name", mAcc->getName()},
        {"resource", mAcc->getResource()}
    });
    acc->lockId();
    connect(acc, SIGNAL(accepted()), this, SLOT(onAccountAccepted()));
    connect(acc, SIGNAL(rejected()), this, SLOT(onAccountRejected()));
    editing = true;
    acc->exec();
}

void Accounts::onSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
    const QItemSelection& selection = m_ui->tableView->selectionModel()->selection();
    if (selection.size() == 0) {
        m_ui->editButton->setEnabled(false);
    } else if (selection.size() == 1) {
        m_ui->editButton->setEnabled(true);
    } else {
        m_ui->editButton->setEnabled(false);
    }
}
