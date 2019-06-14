#include "squawk.h"
#include <QDebug>
#include <QSettings>
#include <QDir>
#include <QStandardPaths>

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
        settings.setValue("resource", acc->getResource());
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
        addAccount(
            settings.value("login").toString(), 
            settings.value("server").toString(), 
            settings.value("password").toString(), 
            settings.value("name").toString(), 
            settings.value("resource").toString()
        );
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
    QString resource = map.value("resource").toString();
    
    addAccount(login, server, password, name, resource);
}

void Core::Squawk::addAccount(const QString& login, const QString& server, const QString& password, const QString& name, const QString& resource)
{
    Account* acc = new Account(login, server, password, name);
    acc->setResource(resource);
    accounts.push_back(acc);
    amap.insert(std::make_pair(name, acc));
    
    connect(acc, SIGNAL(connectionStateChanged(int)), this, SLOT(onAccountConnectionStateChanged(int)));
    connect(acc, SIGNAL(error(const QString&)), this, SLOT(onAccountError(const QString&)));
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
    connect(acc, SIGNAL(message(const Shared::Message&)), this, SLOT(onAccountMessage(const Shared::Message&)));
    connect(acc, SIGNAL(responseArchive(const QString&, const std::list<Shared::Message>&)), 
            this, SLOT(onAccountResponseArchive(const QString&, const std::list<Shared::Message>&)));
    
    QMap<QString, QVariant> map = {
        {"login", login},
        {"server", server},
        {"name", name},
        {"password", password},
        {"resource", resource},
        {"state", Shared::disconnected},
        {"offline", Shared::offline},
        {"error", ""}
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
    emit changeAccount(acc->getName(), {{"state", state}});
    
    if (state == Shared::disconnected) {
        bool equals = true;
        for (Accounts::const_iterator itr = accounts.begin(), end = accounts.end(); itr != end; itr++) {
            if ((*itr)->getState() != Shared::disconnected) {
                equals = false;
            }
        }
        if (equals) {
            state = Shared::offline;
            emit stateChanged(state);
        }
    }
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
    emit changeAccount(acc->getName(), {{"availability", state}});
}

void Core::Squawk::onAccountMessage(const Shared::Message& data)
{
    Account* acc = static_cast<Account*>(sender());
    emit accountMessage(acc->getName(), data);
}

void Core::Squawk::sendMessage(const QString& account, const Shared::Message& data)
{
    AccountsMap::const_iterator itr = amap.find(account);
    if (itr == amap.end()) {
        qDebug("An attempt to send a message with non existing account, skipping");
        return;
    }
    
    itr->second->sendMessage(data);
}

void Core::Squawk::requestArchive(const QString& account, const QString& jid, int count, const QString& before)
{
    AccountsMap::const_iterator itr = amap.find(account);
    if (itr == amap.end()) {
        qDebug("An attempt to request an archive of non existing account, skipping");
        return;
    }
    itr->second->requestArchive(jid, count, before);
}

void Core::Squawk::onAccountResponseArchive(const QString& jid, const std::list<Shared::Message>& list)
{
    Account* acc = static_cast<Account*>(sender());
    emit responseArchive(acc->getName(), jid, list);
}

void Core::Squawk::modifyAccountRequest(const QString& name, const QMap<QString, QVariant>& map)
{
    AccountsMap::const_iterator itr = amap.find(name);
    if (itr == amap.end()) {
        qDebug("An attempt to modify non existing account, skipping");
        return;
    }
    
    Core::Account* acc = itr->second;
    Shared::ConnectionState st = acc->getState();
    
    if (st != Shared::disconnected) {
        acc->reconnect();
    }
    
    QMap<QString, QVariant>::const_iterator mItr;
    mItr = map.find("login");
    if (mItr != map.end()) {
        acc->setLogin(mItr->toString());
    }
    
    mItr = map.find("password");
    if (mItr != map.end()) {
        acc->setPassword(mItr->toString());
    }
    
    mItr = map.find("resource");
    if (mItr != map.end()) {
        acc->setResource(mItr->toString());
    }
    
    mItr = map.find("server");
    if (mItr != map.end()) {
        acc->setServer(mItr->toString());
    }
    
    emit changeAccount(name, map);
}

void Core::Squawk::onAccountError(const QString& text)
{
    Account* acc = static_cast<Account*>(sender());
    emit changeAccount(acc->getName(), {{"error", text}});
}

void Core::Squawk::removeAccountRequest(const QString& name)
{
    AccountsMap::const_iterator itr = amap.find(name);
    if (itr == amap.end()) {
        qDebug() << "An attempt to remove non existing account " << name << " from core, skipping";
        return;
    }
    
    Account* acc = itr->second;
    if (acc->getState() != Shared::disconnected) {
        acc->disconnect();
    }
    
    for (Accounts::const_iterator aItr = accounts.begin(); aItr != accounts.end(); ++aItr) {
        if (*aItr == acc) {
            accounts.erase(aItr);
            break;
        }
    }
    
    amap.erase(itr);
    
    QString path(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
    path += "/" + name;
    QDir dir(path);
    dir.removeRecursively();
    
    emit removeAccount(name);
    acc->deleteLater();
}


void Core::Squawk::subscribeContact(const QString& account, const QString& jid, const QString& reason)
{
    AccountsMap::const_iterator itr = amap.find(account);
    if (itr == amap.end()) {
        qDebug("An attempt to subscribe to the contact with non existing account, skipping");
        return;
    }
    
    itr->second->subscribeToContact(jid, reason);
}

void Core::Squawk::unsubscribeContact(const QString& account, const QString& jid, const QString& reason)
{
    AccountsMap::const_iterator itr = amap.find(account);
    if (itr == amap.end()) {
        qDebug("An attempt to subscribe to the contact with non existing account, skipping");
        return;
    }
    
    itr->second->unsubscribeFromContact(jid, reason);
}

void Core::Squawk::removeContactRequest(const QString& account, const QString& jid)
{
    AccountsMap::const_iterator itr = amap.find(account);
    if (itr == amap.end()) {
        qDebug("An attempt to remove contact from non existing account, skipping");
        return;
    }
    
    itr->second->removeContactRequest(jid);
}
