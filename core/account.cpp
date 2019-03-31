#include "account.h"

using namespace Core;

Account::Account(const QString& p_login, const QString& p_server, const QString& p_password, const QString& p_name, QObject* parent):
    QObject(parent),
    name(p_name),
    login(p_login),
    server(p_server),
    password(p_password),
    client(),
    state(Shared::disconnected)
{
    QObject::connect(&client, SIGNAL(connected()), this, SLOT(onClientConnected()));
    QObject::connect(&client, SIGNAL(disconnected()), this, SLOT(onClientDisconnected()));
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
        client.disconnect();
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

void Core::Account::onClientDisonnected()
{
    if (state != Shared::disconnected) {
        state = Shared::disconnected;
        emit connectionStateChanged(state);
    } else {
        qDebug("Something weird had happened - xmpp client reported about being disconnection but account was already in disconnected state");
    }
}

QString Core::Account::getName() const
{
    return name;
}


