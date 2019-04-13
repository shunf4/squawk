#include "account.h"
#include <qxmpp/QXmppMessage.h>
#include <QDateTime>

using namespace Core;

Account::Account(const QString& p_login, const QString& p_server, const QString& p_password, const QString& p_name, QObject* parent):
    QObject(parent),
    name(p_name),
    achiveQueries(),
    client(),
    config(),
    presence(),
    state(Shared::disconnected),
    groups(),
    cm(new QXmppCarbonManager()),
    am(new QXmppMamManager())
{
    config.setUser(p_login);
    config.setDomain(p_server);
    config.setPassword(p_password);
    
    QObject::connect(&client, SIGNAL(connected()), this, SLOT(onClientConnected()));
    QObject::connect(&client, SIGNAL(disconnected()), this, SLOT(onClientDisconnected()));
    QObject::connect(&client, SIGNAL(presenceReceived(const QXmppPresence&)), this, SLOT(onPresenceReceived(const QXmppPresence&)));
    QObject::connect(&client, SIGNAL(messageReceived(const QXmppMessage&)), this, SLOT(onMessageReceived(const QXmppMessage&)));
    
    QXmppRosterManager& rm = client.rosterManager();
    
    QObject::connect(&rm, SIGNAL(rosterReceived()), this, SLOT(onRosterReceived()));
    QObject::connect(&rm, SIGNAL(itemAdded(const QString&)), this, SLOT(onRosterItemAdded(const QString&)));
    QObject::connect(&rm, SIGNAL(itemRemoved(const QString&)), this, SLOT(onRosterItemRemoved(const QString&)));
    QObject::connect(&rm, SIGNAL(itemChanged(const QString&)), this, SLOT(onRosterItemChanged(const QString&)));
    //QObject::connect(&rm, SIGNAL(presenceChanged(const QString&, const QString&)), this, SLOT(onRosterPresenceChanged(const QString&, const QString&)));
    
    client.addExtension(cm);
    
    QObject::connect(cm, SIGNAL(messageReceived(const QXmppMessage&)), this, SLOT(onCarbonMessageReceived(const QXmppMessage&)));
    QObject::connect(cm, SIGNAL(messageSent(const QXmppMessage&)), this, SLOT(onCarbonMessageSent(const QXmppMessage&)));
    
    client.addExtension(am);
    
    QObject::connect(am, SIGNAL(archivedMessageReceived(const QString&, const QXmppMessage&)), this, SLOT(onMamMessageReceived(const QString&, const QXmppMessage&)));
    QObject::connect(am, SIGNAL(resultsRecieved(const QString&, const QXmppResultSetReply&, bool)), 
                     this, SLOT(onMamResultsReceived(const QString&, const QXmppResultSetReply&, bool)));
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
        client.connectToServer(config, presence);
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
        cm->setCarbonsEnabled(true);
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
    return config.user();
}

QString Core::Account::getPassword() const
{
    return config.password();
}

QString Core::Account::getServer() const
{
    return config.domain();
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
    
    
    QStringList res = rm.getResources(bareJid);
    unsigned int state = re.subscriptionType();
    if (state == QXmppRosterIq::Item::NotSet) {
        state = Shared::unknown;
    }
    QMap<QString, QVariant> cData({
        {"name", re.name()},
        {"state", state}
    });
    
    emit changeContact(bareJid, cData);
    
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
        emit addContact(bareJid, groupName, cData);
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
    unsigned int state = re.subscriptionType();
    if (state == QXmppRosterIq::Item::NotSet) {
        state = Shared::unknown;
    }
    QMap<QString, QVariant> cData({
        {"name", re.name()},
        {"state", state}
    });
    int grCount = 0;
    for (QSet<QString>::const_iterator itr = gr.begin(), end = gr.end(); itr != end; ++itr) {
        const QString& groupName = *itr;
        std::map<QString, std::set<QString>>::iterator gItr = groups.find(groupName);
        if (gItr == groups.end()) {
            gItr = groups.insert(std::make_pair(groupName, std::set<QString>())).first;
            emit addGroup(groupName);
        }
        gItr->second.insert(jid);
        emit addContact(jid, groupName, cData);
        grCount++;
    }
    
    if (grCount == 0) {
        emit addContact(jid, "", cData);
    }
}

void Core::Account::onPresenceReceived(const QXmppPresence& presence)
{
    QString id = presence.from();
    QStringList comps = id.split("/");
    QString jid = comps.front();
    QString resource = comps.back();
    
    QString myJid = getLogin() + "@" + getServer();
    
    if (jid == myJid) {
        if (resource == getResource()) {
            emit availabilityChanged(presence.availableStatusType());
        } else {
            qDebug() << "Received a presence for another resource of my " << name << " account, skipping";
        }
    }
    
    switch (presence.type()) {
        case QXmppPresence::Error:
            qDebug() << "An error reported by presence from " << id;
            break;
        case QXmppPresence::Available:{
            QDateTime lastInteraction = presence.lastUserInteraction();
            if (!lastInteraction.isValid()) {
                lastInteraction = QDateTime::currentDateTime();
            }
            emit addPresence(jid, resource, {
                {"lastActivity", lastInteraction},
                {"availability", presence.availableStatusType()},           //TODO check and handle invisible
                {"status", presence.statusText()}
            });
        }
            break;
        case QXmppPresence::Unavailable:
            emit removePresence(jid, resource);
            break;
        case QXmppPresence::Subscribe:
            qDebug("xmpp presence \"subscribe\" received, do not yet know what to do, skipping");
        case QXmppPresence::Subscribed:
            qDebug("xmpp presence \"subscribed\" received, do not yet know what to do, skipping");
        case QXmppPresence::Unsubscribe:
            qDebug("xmpp presence \"unsubscribe\" received, do not yet know what to do, skipping");
        case QXmppPresence::Unsubscribed:
            qDebug("xmpp presence \"unsubscribed\" received, do not yet know what to do, skipping");
        case QXmppPresence::Probe:
            qDebug("xmpp presence \"probe\" received, do not yet know what to do, skipping");
            break;
    }
}

void Core::Account::onRosterPresenceChanged(const QString& bareJid, const QString& resource)
{
    //not used for now;
    qDebug() << "presence changed for " << bareJid << " resource " << resource;
    const QXmppPresence& presence = client.rosterManager().getPresence(bareJid, resource);
}

void Core::Account::setLogin(const QString& p_login)
{
    config.setUser(p_login);
}

void Core::Account::setName(const QString& p_name)
{
    name = p_name;
}

void Core::Account::setPassword(const QString& p_password)
{
    config.setPassword(p_password);
}

void Core::Account::setServer(const QString& p_server)
{
    config.setDomain(p_server);
}

Shared::Availability Core::Account::getAvailability() const
{
    QXmppPresence::AvailableStatusType pres = presence.availableStatusType();
    return static_cast<Shared::Availability>(pres);         //TODO that's a shame! gotta do something about it
}

void Core::Account::setAvailability(Shared::Availability avail)
{
    QXmppPresence::AvailableStatusType pres = static_cast<QXmppPresence::AvailableStatusType>(avail);
    
    presence.setAvailableStatusType(pres);
    if (state != Shared::disconnected) {        //TODO not sure how to do here - changing state may cause connection or disconnection
        client.setClientPresence(presence);
    }
}

QString Core::Account::getResource() const
{
    return config.resource();
}

void Core::Account::setResource(const QString& p_resource)
{
    config.setResource(p_resource);
}

void Core::Account::onMessageReceived(const QXmppMessage& msg)
{
    bool handled = false;
    switch (msg.type()) {
        case QXmppMessage::Normal:
            qDebug() << "received a message with type \"Normal\", not sure what to do with it now, skipping";
            break;
        case QXmppMessage::Chat:
            handled = handleChatMessage(msg);
            break;
        case QXmppMessage::GroupChat:
            qDebug() << "received a message with type \"GroupChat\", not sure what to do with it now, skipping";
            break;
        case QXmppMessage::Error:
            qDebug() << "received a message with type \"Error\", not sure what to do with it now, skipping";
            break;
        case QXmppMessage::Headline:
            qDebug() << "received a message with type \"Headline\", not sure what to do with it now, skipping";
            break;
    }
    if (!handled) {
        qDebug() << "Message wasn't handled: ";
        qDebug() << "- from: " << msg.from();
        qDebug() << "- to: " << msg.to();
        qDebug() << "- body: " << msg.body();
        qDebug() << "- type: " << msg.type();
        qDebug() << "- state: " << msg.state();
        qDebug() << "- stamp: " << msg.stamp();
        qDebug() << "- id: " << msg.id();
        qDebug() << "- isAttentionRequested: " << msg.isAttentionRequested();
        qDebug() << "- isReceiptRequested: " << msg.isReceiptRequested();
        qDebug() << "- receiptId: " << msg.receiptId();
        qDebug() << "- subject: " << msg.subject();
        qDebug() << "- thread: " << msg.thread();
        qDebug() << "- isMarkable: " << msg.isMarkable();
        qDebug() << "==============================";
    }
}

QString Core::Account::getFullJid() const
{
    return getLogin() + "@" + getServer() + "/" + getResource();
}

void Core::Account::sendMessage(const Shared::Message& data)
{
    if (state == Shared::connected) {
        QXmppMessage msg(data.getFrom(), data.getTo(), data.getBody(), data.getThread());
        msg.setId(data.getId());
        msg.setType(static_cast<QXmppMessage::Type>(data.getType()));       //it is safe here, my type is compatible
        client.sendPacket(msg);
    } else {
        qDebug() << "An attempt to send message with not connected account " << name << ", skipping";
    }
}

void Core::Account::onCarbonMessageReceived(const QXmppMessage& msg)
{
    handleChatMessage(msg, false, true);
}

void Core::Account::onCarbonMessageSent(const QXmppMessage& msg)
{
    handleChatMessage(msg, true, true);
}

bool Core::Account::handleChatMessage(const QXmppMessage& msg, bool outgoing, bool forwarded, bool guessing)
{
    QString body(msg.body());
    if (body.size() != 0) {
        QString id(msg.id());
        QDateTime time(msg.stamp());
        Shared::Message sMsg(Shared::Message::chat);
        sMsg.setId(id);
        sMsg.setFrom(msg.from());
        sMsg.setTo(msg.to());
        sMsg.setBody(body);
        sMsg.setForwarded(forwarded);
        if (guessing) {
            if (sMsg.getFromJid() == getLogin() + "@" + getServer()) {
                outgoing = true;
            } else {
                outgoing = false;
            }
        }
        sMsg.setOutgoing(outgoing);
        if (time.isValid()) {
            sMsg.setTime(time);
        }
        emit message(sMsg);
        
        if (!forwarded && !outgoing) {
            if (msg.isReceiptRequested() && id.size() > 0) {
                QXmppMessage receipt(getFullJid(), msg.from(), "");
                receipt.setReceiptId(id);
                client.sendPacket(receipt);
            }
        }
        
        return true;
    }
    return false;
}

void Core::Account::onMamMessageReceived(const QString& bareJid, const QXmppMessage& msg)
{
    handleChatMessage(msg, false, true, true);
}

void Core::Account::requestAchive(const QString& jid)
{
    QXmppResultSetQuery query;
    query.setMax(100);
    QDateTime from = QDateTime::currentDateTime().addDays(-7);

    QString q = am->retrieveArchivedMessages("", "", jid, from, QDateTime(), query);
    achiveQueries.insert(std::make_pair(q, jid));
}

void Core::Account::onMamResultsReceived(const QString& queryId, const QXmppResultSetReply& resultSetReply, bool complete)
{
    std::map<QString, QString>::const_iterator itr = achiveQueries.find(queryId);
    QString jid = itr->second;
    achiveQueries.erase(itr);
    if (!complete) {
        QXmppResultSetQuery q;
        q.setAfter(resultSetReply.last());
        q.setMax(100);
        QString nQ = am->retrieveArchivedMessages("", "", jid, QDateTime::currentDateTime().addDays(-7), QDateTime(), q);
        achiveQueries.insert(std::make_pair(nQ, jid));
    }
}
