#include "squawk.h"
#include "ui_squawk.h"
#include <QDebug>

Squawk::Squawk(QWidget *parent) :
    QMainWindow(parent),
    m_ui(new Ui::Squawk),
    accounts(0),
    rosterModel()
{
    m_ui->setupUi(this);
    m_ui->roster->setModel(&rosterModel);
    
    connect(m_ui->actionAccounts, SIGNAL(triggered()), this, SLOT(onAccounts()));
    connect(m_ui->comboBox, SIGNAL(activated(int)), this, SLOT(onComboboxActivated(int)));
    //m_ui->mainToolBar->addWidget(m_ui->comboBox);
}

Squawk::~Squawk() {
    
}

void Squawk::onAccounts()
{
    if (accounts == 0) {
        accounts = new Accounts(rosterModel.accountsModel, this);
        accounts->setAttribute(Qt::WA_DeleteOnClose);
        connect(accounts, SIGNAL(destroyed(QObject*)), this, SLOT(onAccountsClosed(QObject*)));
        connect(accounts, SIGNAL(newAccount(const QMap<QString, QVariant>&)), this, SIGNAL(newAccountRequest(const QMap<QString, QVariant>&)));
        
        accounts->show();
    } else {
        accounts->show();
        accounts->raise();
        accounts->activateWindow();
    }
}

void Squawk::closeEvent(QCloseEvent* event)
{
    if (accounts != 0) {
        accounts->close();
    }
    
    QMainWindow::closeEvent(event);
}


void Squawk::onAccountsClosed(QObject* parent)
{
    accounts = 0;
}

void Squawk::newAccount(const QMap<QString, QVariant>& account)
{
    rosterModel.addAccount(account);
}

void Squawk::onComboboxActivated(int index)
{
    if (index == 0) {
        int size = rosterModel.accountsModel->rowCount(QModelIndex());
        if (size > 0) {
            for (int i = 0; i < size; ++i) {
                Models::Account* acc = rosterModel.accountsModel->getAccount(i);
                if (acc->getState() == Shared::disconnected) {
                    emit connectAccount(acc->getName());
                }
            }
        } else {
            m_ui->comboBox->setCurrentIndex(1);
        }
    } else if (index == 1) {
        int size = rosterModel.accountsModel->rowCount(QModelIndex());
        for (int i = 0; i != size; ++i) {
            Models::Account* acc = rosterModel.accountsModel->getAccount(i);
            if (acc->getState() != Shared::disconnected) {
                emit disconnectAccount(acc->getName());
            }
        }
    }
}

void Squawk::accountConnectionStateChanged(const QString& account, int state)
{
    rosterModel.updateAccount(account, "state", state);
}

void Squawk::addContact(const QString& account, const QString& jid, const QString& name, const QString& group)
{
    rosterModel.addContact(account, jid, name, group);
}

void Squawk::addGroup(const QString& account, const QString& name)
{
    rosterModel.addGroup(account, name);
}

void Squawk::removeGroup(const QString& account, const QString& name)
{
    rosterModel.removeGroup(account, name);
}

void Squawk::changeContact(const QString& account, const QString& jid, const QString& name)
{
    rosterModel.changeContact(account, jid, name);
}

void Squawk::removeContact(const QString& account, const QString& jid)
{
    rosterModel.removeContact(account, jid);
}

void Squawk::removeContact(const QString& account, const QString& jid, const QString& group)
{
    rosterModel.removeContact(account, jid, group);
}

void Squawk::addPresence(const QString& account, const QString& jid, const QString& name, const QMap<QString, QVariant>& data)
{
    rosterModel.addPresence(account, jid, name, data);
}

void Squawk::removePresence(const QString& account, const QString& jid, const QString& name)
{
    rosterModel.removePresence(account, jid, name);
}



