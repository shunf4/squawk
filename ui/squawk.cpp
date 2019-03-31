#include "squawk.h"
#include "ui_squawk.h"

Squawk::Squawk(QWidget *parent) :
    QMainWindow(parent),
    m_ui(new Ui::Squawk),
    accounts(0),
    accountsCache(),
    rosterModel()
{
    m_ui->setupUi(this);
    m_ui->roster->setModel(&rosterModel);
    
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
    
}
