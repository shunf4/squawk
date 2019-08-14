/*
 * Squawk messenger. 
 * Copyright (C) 2019  Yury Gubich <blue@macaw.me>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "accounts.h"
#include "ui_accounts.h"

#include <QDebug>

Accounts::Accounts(Models::Accounts* p_model, QWidget *parent) :
    m_ui(new Ui::Accounts),
    model(p_model),
    editing(false),
    toDisconnect(false)
{
    m_ui->setupUi(this);
    
    connect(m_ui->addButton, SIGNAL(clicked(bool)), this, SLOT(onAddButton(bool)));
    connect(m_ui->editButton, SIGNAL(clicked(bool)), this, SLOT(onEditButton(bool)));
    connect(m_ui->connectButton, SIGNAL(clicked(bool)), this, SLOT(onConnectButton(bool)));
    connect(m_ui->deleteButton, SIGNAL(clicked(bool)), this, SLOT(onDeleteButton(bool)));
    m_ui->tableView->setModel(model);
    connect(m_ui->tableView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
            this, SLOT(onSelectionChanged(const QItemSelection&, const QItemSelection&)));
    connect(p_model, SIGNAL(changed()), this, SLOT(updateConnectButton()));
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
        const Models::Account* mAcc = model->getAccount(m_ui->tableView->selectionModel()->selectedRows().at(0).row());
        emit changeAccount(mAcc->getName(), map);
    } else {
        emit newAccount(map);
    }
    
    acc->deleteLater();
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
    int selectionSize = m_ui->tableView->selectionModel()->selection().size();
    if (selectionSize == 0) {
        m_ui->editButton->setEnabled(false);
        m_ui->deleteButton->setEnabled(false);
    } else if (selectionSize == 1) {
        m_ui->editButton->setEnabled(true);
        m_ui->deleteButton->setEnabled(true);
    } else {
        m_ui->editButton->setEnabled(false);
        m_ui->deleteButton->setEnabled(true);
    }
    
    updateConnectButton();
}

void Accounts::updateConnectButton()
{
    QItemSelectionModel* sm = m_ui->tableView->selectionModel();
    if (sm->hasSelection()) {
        m_ui->connectButton->setEnabled(true);
        int selectionSize = sm->selection().size();
        bool allConnected = true;
        for (int i = 0; i < selectionSize && allConnected; ++i) {
            const Models::Account* mAcc = model->getAccount(sm->selectedRows().at(i).row());
            allConnected = mAcc->getState() == Shared::connected;
        }
        if (allConnected) {
            toDisconnect = true;
            m_ui->connectButton->setText("Disconnect");
        } else {
            toDisconnect = false;
            m_ui->connectButton->setText("Connect");
        }
    } else {
        m_ui->connectButton->setText("Connect");
        toDisconnect = false;
        m_ui->connectButton->setEnabled(false);
    }
}

void Accounts::onConnectButton(bool clicked)
{
    QItemSelectionModel* sm = m_ui->tableView->selectionModel();
    int selectionSize = sm->selection().size();
    for (int i = 0; i < selectionSize; ++i) {
        const Models::Account* mAcc = model->getAccount(sm->selectedRows().at(i).row());
        if (toDisconnect) {
            emit disconnectAccount(mAcc->getName());
        } else {
            emit connectAccount(mAcc->getName());
        }
    }
}

void Accounts::onDeleteButton(bool clicked)
{
    QItemSelectionModel* sm = m_ui->tableView->selectionModel();
    int selectionSize = sm->selection().size();
    for (int i = 0; i < selectionSize; ++i) {
        const Models::Account* mAcc = model->getAccount(sm->selectedRows().at(i).row());
        emit removeAccount(mAcc->getName());
    }
}
