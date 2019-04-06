#include "account.h"
#include <qxmpp/QXmppRosterManager.h>

using namespace Core;

Account::Account(const QString& p_login, const QString& p_server, const QString& p_password, const QString& p_name, QObject* parent):
    QObject(parent),
    name(p_name),
    login(p_login),
    server(p_server),
    password(p_password),
    client(),
    state(Shared::disconnected),
    groups()
{
    QObject::connect(&client, SIGNAL(connected()), this, SLOT(onClientConnected()));
    QObject::connect(&client, SIGNAL(disconnected()), this, SLOT(onClientDisconnected()));
    
    
    QXmppRosterManager& rm = client.rosterManager();
    
    QObject::connect(&rm, SIGNAL(rosterReceived()), this, SLOT(onRosterReceived()));
    QObject::connect(&rm, SIGNAL(itemAdded(const QString&)), this, SLOT(onRosterItemAdded(const QString&)));
    QObject::connect(&rm, SIGNAL(itemRemoved(const QString&)), this, SLOT(onRosterItemRemoved(const QString&)));
    QObject::connect(&rm, SIGNAL(itemChanged(const QString&)), this, SLOT(onRosterItemChanged(const QString&)));
}

Account::~Account()
{
}

Shared::ConnectionState Core::Account::getState() const
{
    return state;
}

void Core::Account::connect()
{
    if (state == Shared::disconnected) {
        client.connectToServer(login + "@" + server, password);
        state = Shared::connecting;
        emit connectionStateChanged(state);
    } else {
        qDebug("An attempt to connect an account which is already connected, skipping");
    }
}

void Core::Account::disconnect()
{
    if (state != Shared::disconnected) {
        client.disconnectFromServer();
        state = Shared::disconnected;
        emit connectionStateChanged(state);
    }
}

void Core::Account::onClientConnected()
{
    if (state == Shared::connecting) {
        state = Shared::connected;
        emit connectionStateChanged(state);
    } else {
        qDebug("Something weird had happened - xmpp client reported about successful connection but account wasn't in connecting state");
    }
}

void Core::Account::onClientDisconnected()
{
    if (state != Shared::disconnected) {
        state = Shared::disconnected;
        emit connectionStateChanged(state);
    } else {
        //qDebug("Something weird had happened - xmpp client reported about being disconnection but account was already in disconnected state");
    }
}

QString Core::Account::getName() const
{
    return name;
}

QString Core::Account::getLogin() const
{
    return login;
}

QString Core::Account::getPassword() const
{
    return password;
}

QString Core::Account::getServer() const
{
    return server;
}

void Core::Account::onRosterReceived()
{
    QXmppRosterManager& rm = client.rosterManager();
    QStringList bj = rm.getRosterBareJids();
    for (int i = 0; i < bj.size(); ++i) {
        const QString& jid = bj[i];
        addedAccount(jid);
    }
}

void Core::Account::onRosterItemAdded(const QString& bareJid)
{
    addedAccount(bareJid);
}

void Core::Account::onRosterItemChanged(const QString& bareJid)
{
    QXmppRosterManager& rm = client.rosterManager();
    QXmppRosterIq::Item re = rm.getRosterEntry(bareJid);
    QSet<QString> newGroups = re.groups();
    QSet<QString> oldGroups;
    
    emit changeContact(bareJid, re.name());
    
    for (std::map<QString, std::set<QString>>::iterator itr = groups.begin(), end = groups.end(); itr != end; ++itr) {
        std::set<QString>& contacts = itr->second;
        std::set<QString>::const_iterator cItr = contacts.find(bareJid);
        if (cItr != contacts.end()) {
            oldGroups.insert(itr->first);
        }
    }
    
    QSet<QString> toRemove = oldGroups - newGroups;
    QSet<QString> toAdd = newGroups - oldGroups;
    
    QSet<QString> removeGroups;
    for (QSet<QString>::iterator itr = toRemove.begin(), end = toRemove.end(); itr != end; ++itr) {
        const QString& groupName = *itr;
        std::set<QString>& contacts = groups.find(groupName)->second;
        contacts.erase(bareJid);
        emit removeContact(bareJid, groupName);
        if (contacts.size() == 0) {
            removeGroups.insert(groupName);
        }
    }
    
    for (QSet<QString>::iterator itr = toAdd.begin(), end = toAdd.end(); itr != end; ++itr) {
        const QString& groupName = *itr;
        std::map<QString, std::set<QString>>::iterator cItr = groups.find(groupName);
        if (cItr == groups.end()) {
            cItr = groups.insert(std::make_pair(groupName, std::set<QString>())).first;
            emit addGroup(groupName);
        }
        cItr->second.insert(bareJid);
        emit addContact(bareJid, re.name(), groupName);
    }
    
    for (QSet<QString>::iterator itr = removeGroups.begin(), end = removeGroups.end(); itr != end; ++itr) {
        const QString& groupName = *itr;
        emit removeGroup(groupName);
        groups.erase(groupName);
    }
}

void Core::Account::onRosterItemRemoved(const QString& bareJid)
{
    emit removeContact(bareJid);
    
    QSet<QString> toRemove;
    for (std::map<QString, std::set<QString>>::iterator itr = groups.begin(), end = groups.end(); itr != end; ++itr) {
        std::set<QString> contacts = itr->second;
        std::set<QString>::const_iterator cItr = contacts.find(bareJid);
        if (cItr != contacts.end()) {
            contacts.erase(cItr);
            if (contacts.size() == 0) {
                toRemove.insert(itr->first);
            }
        }
    }
    
    for (QSet<QString>::iterator itr = toRemove.begin(), end = toRemove.end(); itr != end; ++itr) {
        const QString& groupName = *itr;
        emit removeGroup(groupName);
        groups.erase(groupName);
    }
}

void Core::Account::addedAccount(const QString& jid)
{
    QXmppRosterManager& rm = client.rosterManager();
    QXmppRosterIq::Item re = rm.getRosterEntry(jid);
    QSet<QString> gr = re.groups();
    int grCount = 0;
    for (QSet<QString>::const_iterator itr = gr.begin(), end = gr.end(); itr != end; ++itr) {
        const QString& groupName = *itr;
        std::map<QString, std::set<QString>>::iterator gItr = groups.find(groupName);
        if (gItr == groups.end()) {
            gItr = groups.insert(std::make_pair(groupName, std::set<QString>())).first;
            emit addGroup(groupName);
        }
        gItr->second.insert(jid);
        emit addContact(jid, re.name(), groupName);
        grCount++;
    }
    
    if (grCount == 0) {
        emit addContact(jid, re.name(), "");
    }
}

