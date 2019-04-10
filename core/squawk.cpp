#include "squawk.h"
#include <QDebug>
#include <QSettings>

Core::Squawk::Squawk(QObject* parent):
    QObject(parent),
    accounts(),
    amap()
{

}

Core::Squawk::~Squawk()
{
    Accounts::const_iterator itr = accounts.begin(); 
    Accounts::const_iterator end = accounts.end();
    for (; itr != end; ++itr) {
        delete (*itr);
    }
}

void Core::Squawk::stop()
{
    qDebug("Stopping squawk core..");
    
    QSettings settings;
    settings.beginGroup("core");
    settings.beginWriteArray("accounts");
    for (int i = 0; i < accounts.size(); ++i) {
        settings.setArrayIndex(i);
        Account* acc = accounts[i];
        settings.setValue("name", acc->getName());
        settings.setValue("server", acc->getServer());
        settings.setValue("login", acc->getLogin());
        settings.setValue("password", acc->getPassword());
    }
    settings.endArray();
    settings.endGroup();
    
    settings.sync();
    
    emit quit();
}

void Core::Squawk::start()
{
    qDebug("Starting squawk core..");
    
    QSettings settings;
    settings.beginGroup("core");
    int size = settings.beginReadArray("accounts");
    for (int i = 0; i < size; ++i) {
        settings.setArrayIndex(i);
        addAccount(settings.value("login").toString(), settings.value("server").toString(), settings.value("password").toString(), settings.value("name").toString());
    }
    settings.endArray();
    settings.endGroup();
}

void Core::Squawk::newAccountRequest(const QMap<QString, QVariant>& map)
{
    QString name = map.value("name").toString();
    QString login = map.value("login").toString();
    QString server = map.value("server").toString();
    QString password = map.value("password").toString();
    
    addAccount(login, server, password, name);
}

void Core::Squawk::addAccount(const QString& login, const QString& server, const QString& password, const QString& name)
{
    Account* acc = new Account(login, server, password, name);
    accounts.push_back(acc);
    amap.insert(std::make_pair(name, acc));
    
    connect(acc, SIGNAL(connectionStateChanged(int)), this, SLOT(onAccountConnectionStateChanged(int)));
    connect(acc, SIGNAL(availabilityChanged(int)), this, SLOT(onAccountAvailabilityChanged(int)));
    connect(acc, SIGNAL(addContact(const QString&, const QString&, const QMap<QString, QVariant>&)), 
            this, SLOT(onAccountAddContact(const QString&, const QString&, const QMap<QString, QVariant>&)));
    connect(acc, SIGNAL(addGroup(const QString&)), this, SLOT(onAccountAddGroup(const QString&)));
    connect(acc, SIGNAL(removeGroup(const QString&)), this, SLOT(onAccountRemoveGroup(const QString&)));
    connect(acc, SIGNAL(removeContact(const QString&)), this, SLOT(onAccountRemoveContact(const QString&)));
    connect(acc, SIGNAL(removeContact(const QString&, const QString&)), this, SLOT(onAccountRemoveContact(const QString&, const QString&)));
    connect(acc, SIGNAL(changeContact(const QString&, const QMap<QString, QVariant>&)), 
            this, SLOT(onAccountChangeContact(const QString&, const QMap<QString, QVariant>&)));
    connect(acc, SIGNAL(addPresence(const QString&, const QString&, const QMap<QString, QVariant>&)), 
            this, SLOT(onAccountAddPresence(const QString&, const QString&, const QMap<QString, QVariant>&)));
    connect(acc, SIGNAL(removePresence(const QString&, const QString&)), this, SLOT(onAccountRemovePresence(const QString&, const QString&)));
    connect(acc, SIGNAL(message(const QMap<QString, QString>&)), this, SLOT(onAccountMessage(const QMap<QString, QString>&)));
    
    QMap<QString, QVariant> map = {
        {"login", login},
        {"server", server},
        {"name", name},
        {"password", password},
        {"state", Shared::disconnected},
        {"offline", Shared::offline}
    };
    emit newAccount(map);
}

void Core::Squawk::changeState(int p_state)
{
    Shared::Availability avail;
    if (p_state < Shared::availabilityLowest && p_state > Shared::availabilityHighest) {
        qDebug("An attempt to set invalid availability to Squawk core, skipping");
    }
    avail = static_cast<Shared::Availability>(p_state);
    state = avail;
    
    for (std::deque<Account*>::iterator itr = accounts.begin(), end = accounts.end(); itr != end; ++itr) {
        (*itr)->setAvailability(state);
    }
}

void Core::Squawk::connectAccount(const QString& account)
{
    AccountsMap::const_iterator itr = amap.find(account);
    if (itr == amap.end()) {
        qDebug("An attempt to connect non existing account, skipping");
        return;
    }
    itr->second->connect();
}

void Core::Squawk::disconnectAccount(const QString& account)
{
    AccountsMap::const_iterator itr = amap.find(account);
    if (itr == amap.end()) {
        qDebug("An attempt to connect non existing account, skipping");
        return;
    }
    
    itr->second->disconnect();
}

void Core::Squawk::onAccountConnectionStateChanged(int state)
{
    Account* acc = static_cast<Account*>(sender());
    emit accountConnectionStateChanged(acc->getName(), state);
}

void Core::Squawk::onAccountAddContact(const QString& jid, const QString& group, const QMap<QString, QVariant>& data)
{
    Account* acc = static_cast<Account*>(sender());
    emit addContact(acc->getName(), jid, group, data);
}

void Core::Squawk::onAccountAddGroup(const QString& name)
{
    Account* acc = static_cast<Account*>(sender());
    emit addGroup(acc->getName(), name);
}

void Core::Squawk::onAccountRemoveGroup(const QString& name)
{
    Account* acc = static_cast<Account*>(sender());
    emit removeGroup(acc->getName(), name);
}

void Core::Squawk::onAccountChangeContact(const QString& jid, const QMap<QString, QVariant>& data)
{
    Account* acc = static_cast<Account*>(sender());
    emit changeContact(acc->getName(), jid, data);
}

void Core::Squawk::onAccountRemoveContact(const QString& jid)
{
    Account* acc = static_cast<Account*>(sender());
    emit removeContact(acc->getName(), jid);
}

void Core::Squawk::onAccountRemoveContact(const QString& jid, const QString& group)
{
    Account* acc = static_cast<Account*>(sender());
    emit removeContact(acc->getName(), jid, group);
}

void Core::Squawk::onAccountAddPresence(const QString& jid, const QString& name, const QMap<QString, QVariant>& data)
{
    Account* acc = static_cast<Account*>(sender());
    emit addPresence(acc->getName(), jid, name, data);
}

void Core::Squawk::onAccountRemovePresence(const QString& jid, const QString& name)
{
    Account* acc = static_cast<Account*>(sender());
    emit removePresence(acc->getName(), jid, name);
}

void Core::Squawk::onAccountAvailabilityChanged(int state)
{
    Account* acc = static_cast<Account*>(sender());
    emit accountAvailabilityChanged(acc->getName(), state);
}

void Core::Squawk::onAccountMessage(const QMap<QString, QString>& data)
{
    Account* acc = static_cast<Account*>(sender());
    emit accountMessage(acc->getName(), data);
}

void Core::Squawk::sendMessage(const QString& account, const QMap<QString, QString>& data)
{
    AccountsMap::const_iterator itr = amap.find(account);
    if (itr == amap.end()) {
        qDebug("An attempt to send a message with non existing account, skipping");
        return;
    }
    
    itr->second->sendMessage(data);
}
