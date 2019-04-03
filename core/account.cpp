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
        QXmppRosterIq::Item re = rm.getRosterEntry(jid);
        QSet<QString> gr = re.groups();
        int grCount = 0;
        for (QSet<QString>::const_iterator itr = gr.begin(), end = gr.end(); itr != end; ++itr) {
            const QString& groupName = *itr;
            std::map<QString, int>::iterator gItr = groups.find(groupName);
            if (gItr == groups.end()) {
                gItr = groups.insert(std::make_pair(groupName, 0)).first;
                emit addGroup(groupName);
            }
            gItr->second++;
            emit addContact(jid, re.name(), groupName);
            grCount++;
        }
        
        if (grCount == 0) {
            emit addContact(jid, re.name(), "");
        }
    }
}

