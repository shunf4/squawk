#include "squawk.h"
#include "ui_squawk.h"

Squawk::Squawk(QWidget *parent) :
    QMainWindow(parent),
    m_ui(new Ui::Squawk),
    accounts(0),
    accountsCache()
{
    m_ui->setupUi(this);
    
    connect(m_ui->actionAccounts, SIGNAL(triggered()), this, SLOT(onAccounts()));
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
    if (accounts != 0) {
        accounts->addAccount(account);
    }
}
