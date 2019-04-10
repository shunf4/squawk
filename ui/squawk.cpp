#include "squawk.h"
#include "ui_squawk.h"
#include <QDebug>
#include <QIcon>

Squawk::Squawk(QWidget *parent) :
    QMainWindow(parent),
    m_ui(new Ui::Squawk),
    accounts(0),
    rosterModel(),
    conversations()
{
    m_ui->setupUi(this);
    m_ui->roster->setModel(&rosterModel);
    
    for (int i = 0; i < Shared::availabilityNames.size(); ++i) {
        m_ui->comboBox->addItem(QIcon::fromTheme(Shared::availabilityThemeIcons[i]), Shared::availabilityNames[i]);
    }
    m_ui->comboBox->setCurrentIndex(Shared::offline);
    
    connect(m_ui->actionAccounts, SIGNAL(triggered()), this, SLOT(onAccounts()));
    connect(m_ui->comboBox, SIGNAL(activated(int)), this, SLOT(onComboboxActivated(int)));
    connect(m_ui->roster, SIGNAL(doubleClicked(const QModelIndex&)), this, SLOT(onRosterItemDoubleClicked(const QModelIndex&)));
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
    for (Conversations::const_iterator itr = conversations.begin(), end = conversations.end(); itr != end; ++itr) {
        disconnect(itr->second, SIGNAL(destroyed(QObject*)), this, SLOT(onConversationClosed(QObject*)));
        itr->second->close();
    }
    conversations.clear();
    
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
    if (index != Shared::offline) {
        int size = rosterModel.accountsModel->rowCount(QModelIndex());
        if (size > 0) {
            emit changeState(index);
            for (int i = 0; i < size; ++i) {
                Models::Account* acc = rosterModel.accountsModel->getAccount(i);
                if (acc->getState() == Shared::disconnected) {
                    emit connectAccount(acc->getName());
                }
            }
        } else {
            m_ui->comboBox->setCurrentIndex(Shared::offline);
        }
    } else {
        emit changeState(index);
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

void Squawk::accountAvailabilityChanged(const QString& account, int state)
{
    rosterModel.updateAccount(account, "availability", state);
}

void Squawk::addContact(const QString& account, const QString& jid, const QString& group, const QMap<QString, QVariant>& data)
{
    rosterModel.addContact(account, jid, group, data);
}

void Squawk::addGroup(const QString& account, const QString& name)
{
    rosterModel.addGroup(account, name);
}

void Squawk::removeGroup(const QString& account, const QString& name)
{
    rosterModel.removeGroup(account, name);
}

void Squawk::changeContact(const QString& account, const QString& jid, const QMap<QString, QVariant>& data)
{
    rosterModel.changeContact(account, jid, data);
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

void Squawk::stateChanged(int state)
{
    m_ui->comboBox->setCurrentIndex(state);
}

void Squawk::onRosterItemDoubleClicked(const QModelIndex& item)
{
    if (item.isValid()) {
        Models::Item* node = static_cast<Models::Item*>(item.internalPointer());
        Models::Contact* contact = 0;
        switch (node->type) {
            case Models::Item::contact:
                contact = static_cast<Models::Contact*>(node);
                break;
            case Models::Item::presence:
                contact = static_cast<Models::Contact*>(node->parentItem());
                break;
            default:
                m_ui->roster->expand(item);
                break;
        }
        
        if (contact != 0) {
            QString jid = contact->getJid();
            QString account = contact->getAccountName();
            Models::Roster::ElId id(account, jid);
            Conversations::const_iterator itr = conversations.find(id);
            if (itr != conversations.end()) {
                itr->second->show();
                itr->second->raise();
                itr->second->activateWindow();
            } else {
                Conversation* conv = new Conversation(contact);
                
                conv->setAttribute(Qt::WA_DeleteOnClose);
                connect(conv, SIGNAL(destroyed(QObject*)), this, SLOT(onConversationClosed(QObject*)));
                connect(conv, SIGNAL(sendMessage(const QString&)), this, SLOT(onConversationMessage(const QString&)));
                
                conversations.insert(std::make_pair(id, conv));
                rosterModel.dropMessages(account, jid);
                
                conv->show();
            }
        }
    }
}

void Squawk::onConversationClosed(QObject* parent)
{
    Conversation* conv = static_cast<Conversation*>(sender());
    Conversations::const_iterator itr = conversations.find({conv->getAccount(), conv->getJid()});
    if (itr == conversations.end()) {
        qDebug() << "Conversation has been closed but can not be found among other opened conversations, application is most probably going to crash";
        return;
    }
    conversations.erase(itr);
}

void Squawk::accountMessage(const QString& account, const QMap<QString, QString>& data)
{
    const QString& from = data.value("from");
    Conversations::iterator itr = conversations.find({account, from});
    if (itr != conversations.end()) {
        qDebug() << "adding message";
        itr->second->addMessage(data);
    } else {
        qDebug() << "pending message";
        rosterModel.addMessage(account, data);
    }
}

void Squawk::onConversationMessage(const QString& item)
{
    Conversation* conv = static_cast<Conversation*>(sender());
    emit sendMessage(conv->getAccount(), {
        {"to", conv->getJid()},
        {"body", item}
    });
}
