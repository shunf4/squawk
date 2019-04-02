#include "squawk.h"
#include "ui_squawk.h"
#include <QDebug>

Squawk::Squawk(QWidget *parent) :
    QMainWindow(parent),
    m_ui(new Ui::Squawk),
    accounts(0),
    accountsCache(),
    accountsIndex(),
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
        accounts = new Accounts(this);
        accounts->setAttribute(Qt::WA_DeleteOnClose);
        connect(accounts, SIGNAL(destroyed(QObject*)), this, SLOT(onAccountsClosed(QObject*)));
        connect(accounts, SIGNAL(newAccount(const QMap<QString, QVariant>&)), this, SIGNAL(newAccountRequest(const QMap<QString, QVariant>&)));
        
        AC::const_iterator itr = accountsCache.begin();
        AC::const_iterator end = accountsCache.end();
        
        for (; itr != end; ++itr) {
            accounts->addAccount(*itr);
        }
        
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
    accountsCache.push_back(account);
    QMap<QString, QVariant>* acc = &accountsCache.back();
    accountsIndex.insert(std::make_pair(acc->value("name").toString(), acc));
    rosterModel.addAccount(account);
    if (accounts != 0) {
        accounts->addAccount(account);
    }
}

void Squawk::onComboboxActivated(int index)
{
    if (index == 0) {
        if (accountsCache.size() > 0) {
            AC::const_iterator itr = accountsCache.begin();
            AC::const_iterator end = accountsCache.end();
            
            for (; itr != end; ++itr) {
                const QMap<QString, QVariant>& acc = *itr;
                if (acc.value("state").toInt() == Shared::disconnected) {
                    emit connectAccount(acc.value("name").toString());
                }
            }
        } else {
            m_ui->comboBox->setCurrentIndex(1);
        }
    } else if (index == 1) {
        AC::const_iterator itr = accountsCache.begin();
        AC::const_iterator end = accountsCache.end();
        
        for (; itr != end; ++itr) {
            const QMap<QString, QVariant>& acc = *itr;
            if (acc.value("state").toInt() != Shared::disconnected) {
                emit disconnectAccount(acc.value("name").toString());
            }
        }
    }
}

void Squawk::accountConnectionStateChanged(const QString& account, int state)
{
    AI::iterator itr = accountsIndex.find(account);
    if (itr != accountsIndex.end()) {
        QMap<QString, QVariant>* acc = itr->second;
        acc->insert("state", state);
        
        rosterModel.updateAccount(account, "state", state);
        if (accounts != 0) {
            accounts->updateAccount(account, "state", state);
        }
    } else {
        QString msg("A notification about connection state change of an unknown account ");
        msg += account + ", skipping";
        qDebug("%s", msg.toStdString().c_str());
    }
}
