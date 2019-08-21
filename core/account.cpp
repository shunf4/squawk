/*
 * Squawk messenger. 
 * Copyright (C) 2019 Yury Gubich <blue@macaw.me>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "account.h"
#include <QXmppMessage.h>
#include <QDateTime>
#include <QTimer>

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
    am(new QXmppMamManager()),
    mm(new QXmppMucManager()),
    bm(new QXmppBookmarkManager()),
    contacts(),
    conferences(),
    maxReconnectTimes(0),
    reconnectTimes(0),
    queuedContacts(),
    outOfRosterContacts()
{
    config.setUser(p_login);
    config.setDomain(p_server);
    config.setPassword(p_password);
    config.setAutoAcceptSubscriptions(true);
    
    QObject::connect(&client, SIGNAL(connected()), this, SLOT(onClientConnected()));
    QObject::connect(&client, SIGNAL(disconnected()), this, SLOT(onClientDisconnected()));
    QObject::connect(&client, SIGNAL(presenceReceived(const QXmppPresence&)), this, SLOT(onPresenceReceived(const QXmppPresence&)));
    QObject::connect(&client, SIGNAL(messageReceived(const QXmppMessage&)), this, SLOT(onMessageReceived(const QXmppMessage&)));
    QObject::connect(&client, SIGNAL(error(QXmppClient::Error)), this, SLOT(onClientError(QXmppClient::Error)));
    
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
    
    QObject::connect(am, SIGNAL(logMessage(QXmppLogger::MessageType, const QString&)), this, SLOT(onMamLog(QXmppLogger::MessageType, const QString)));
    QObject::connect(am, SIGNAL(archivedMessageReceived(const QString&, const QXmppMessage&)), this, SLOT(onMamMessageReceived(const QString&, const QXmppMessage&)));
    QObject::connect(am, SIGNAL(resultsRecieved(const QString&, const QXmppResultSetReply&, bool)), 
                     this, SLOT(onMamResultsReceived(const QString&, const QXmppResultSetReply&, bool)));
    
    client.addExtension(mm);
    QObject::connect(mm, SIGNAL(roomAdded(QXmppMucRoom*)), this, SLOT(onMucRoomAdded(QXmppMucRoom*)));
    
    client.addExtension(bm);
    QObject::connect(bm, SIGNAL(bookmarksReceived(const QXmppBookmarkSet&)), this, SLOT(bookmarksReceived(const QXmppBookmarkSet&)));
}

Account::~Account()
{
    for (std::map<QString, Contact*>::const_iterator itr = contacts.begin(), end = contacts.end(); itr != end; ++itr) {
        delete itr->second;
    }
    
    for (std::map<QString, Conference*>::const_iterator itr = conferences.begin(), end = conferences.end(); itr != end; ++itr) {
        delete itr->second;
    }
    
    delete bm;
    delete mm;
    delete am;
    delete cm;
}

Shared::ConnectionState Core::Account::getState() const
{
    return state;
}

void Core::Account::connect()
{
    if (state == Shared::disconnected) {
        reconnectTimes = maxReconnectTimes;
        state = Shared::connecting;
        client.connectToServer(config, presence);
        emit connectionStateChanged(state);
    } else {
        qDebug("An attempt to connect an account which is already connected, skipping");
    }
}

void Core::Account::disconnect()
{
    reconnectTimes = 0;
    if (state != Shared::disconnected) {
        client.disconnectFromServer();
        state = Shared::disconnected;
        emit connectionStateChanged(state);
    }
}

void Core::Account::onClientConnected()
{
    if (state == Shared::connecting) {
        reconnectTimes = maxReconnectTimes;
        state = Shared::connected;
        cm->setCarbonsEnabled(true);
        emit connectionStateChanged(state);
    } else {
        qDebug() << "Something weird had happened - xmpp client reported about successful connection but account wasn't in" << state << "state";
    }
}

void Core::Account::onClientDisconnected()
{
    if (state != Shared::disconnected) {
        if (reconnectTimes > 0) {
            --reconnectTimes;
            qDebug() << "Reconnecting...";
            state = Shared::connecting;
            client.connectToServer(config, presence);
            emit connectionStateChanged(state);
        } else {
            state = Shared::disconnected;
            emit connectionStateChanged(state);
        }
    } else {
        //qDebug("Something weird had happened - xmpp client reported about being disconnection but account was already in disconnected state");
    }
}

void Core::Account::reconnect()
{
    if (state == Shared::connected) {
        ++reconnectTimes;
        client.disconnectFromServer();
    } else {
        qDebug() << "An attempt to reconnect account" << getName() << "which was not connected";
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

void Core::Account::setReconnectTimes(unsigned int times)
{
    maxReconnectTimes = times;
    if (state == Shared::connected) {
        reconnectTimes = times;
    }
}

void Core::Account::onRosterItemAdded(const QString& bareJid)
{
    addedAccount(bareJid);
    std::map<QString, QString>::const_iterator itr = queuedContacts.find(bareJid);
    if (itr != queuedContacts.end()) {
        QXmppRosterManager& rm = client.rosterManager();
        rm.subscribe(bareJid, itr->second);
        queuedContacts.erase(itr);
    }
}

void Core::Account::onRosterItemChanged(const QString& bareJid)
{
    std::map<QString, Contact*>::const_iterator itr = contacts.find(bareJid);
    if (itr == contacts.end()) {
        qDebug() << "An attempt to change non existing contact" << bareJid << "from account" << name << ", skipping";
        return;
    }
    Contact* contact = itr->second;
    QXmppRosterManager& rm = client.rosterManager();
    QXmppRosterIq::Item re = rm.getRosterEntry(bareJid);
    
    QStringList res = rm.getResources(bareJid);
    Shared::SubscriptionState state = castSubscriptionState(re.subscriptionType());

    contact->setGroups(re.groups());
    contact->setSubscriptionState(state);
    contact->setName(re.name());
}

void Core::Account::onRosterItemRemoved(const QString& bareJid)
{
    std::map<QString, Contact*>::const_iterator itr = contacts.find(bareJid);
    if (itr == contacts.end()) {
        qDebug() << "An attempt to remove non existing contact" << bareJid << "from account" << name << ", skipping";
        return;
    }
    Contact* contact = itr->second;
    contacts.erase(itr);
    QSet<QString> cGroups = contact->getGroups();
    for (QSet<QString>::const_iterator itr = cGroups.begin(), end = cGroups.end(); itr != end; ++itr) {
        removeFromGroup(bareJid, *itr);
    }
    emit removeContact(bareJid);
    
    contact->deleteLater();
}

void Core::Account::addedAccount(const QString& jid)
{
    QXmppRosterManager& rm = client.rosterManager();
    std::map<QString, Contact*>::const_iterator itr = contacts.find(jid);
    QXmppRosterIq::Item re = rm.getRosterEntry(jid);
    Contact* contact;
    bool newContact = false;
    if (itr == contacts.end()) {
        newContact = true;
        contact = new Contact(jid, name);
        contacts.insert(std::make_pair(jid, contact));
        
    } else {
        contact = itr->second;
    }
    
    QSet<QString> gr = re.groups();
    Shared::SubscriptionState state = castSubscriptionState(re.subscriptionType());
    contact->setGroups(gr);
    contact->setSubscriptionState(state);
    contact->setName(re.name());
    
    if (newContact) {
        QMap<QString, QVariant> cData({
            {"name", re.name()},
            {"state", state}
        });
        int grCount = 0;
        for (QSet<QString>::const_iterator itr = gr.begin(), end = gr.end(); itr != end; ++itr) {
            const QString& groupName = *itr;
            addToGroup(jid, groupName);
            emit addContact(jid, groupName, cData);
            grCount++;
        }
        
        if (grCount == 0) {
            emit addContact(jid, "", cData);
        }
        handleNewContact(contact);
    }
}

void Core::Account::handleNewRosterItem(Core::RosterItem* contact)
{
    
    QObject::connect(contact, SIGNAL(needHistory(const QString&, const QString&, const QDateTime&)), this, SLOT(onContactNeedHistory(const QString&, const QString&, const QDateTime&)));
    QObject::connect(contact, SIGNAL(historyResponse(const std::list<Shared::Message>&)), this, SLOT(onContactHistoryResponse(const std::list<Shared::Message>&)));
    QObject::connect(contact, SIGNAL(nameChanged(const QString&)), this, SLOT(onContactNameChanged(const QString&)));
}

void Core::Account::handleNewContact(Core::Contact* contact)
{
    handleNewRosterItem(contact);
    QObject::connect(contact, SIGNAL(groupAdded(const QString&)), this, SLOT(onContactGroupAdded(const QString&)));
    QObject::connect(contact, SIGNAL(groupRemoved(const QString&)), this, SLOT(onContactGroupRemoved(const QString&)));
    QObject::connect(contact, SIGNAL(subscriptionStateChanged(Shared::SubscriptionState)), 
                     this, SLOT(onContactSubscriptionStateChanged(Shared::SubscriptionState)));
}

void Core::Account::handleNewConference(Core::Conference* contact)
{
    handleNewRosterItem(contact);
    QObject::connect(contact, SIGNAL(nickChanged(const QString&)), this, SLOT(onMucNickNameChanged(const QString&)));
    QObject::connect(contact, SIGNAL(joinedChanged(bool)), this, SLOT(onMucJoinedChanged(bool)));
    QObject::connect(contact, SIGNAL(autoJoinChanged(bool)), this, SLOT(onMucAutoJoinChanged(bool)));
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
    if (state == Shared::connected) {
        QXmppPresence::AvailableStatusType pres = presence.availableStatusType();
        return static_cast<Shared::Availability>(pres);         //they are compatible;
    } else {
        return Shared::offline;
    }
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
        logMessage(msg);
    }
}

void Core::Account::logMessage(const QXmppMessage& msg, const QString& reason)
{
    qDebug() << reason;
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
        
        std::map<QString, Contact*>::const_iterator itr = contacts.find(data.getPenPalJid());
        
        itr->second->appendMessageToArchive(data);
        
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
    const QString& body(msg.body());
    if (body.size() != 0) {
        const QString& id(msg.id());
        Shared::Message sMsg(Shared::Message::chat);
        initializeMessage(sMsg, msg, outgoing, forwarded, guessing);
        QString jid = sMsg.getPenPalJid();
        std::map<QString, Contact*>::const_iterator itr = contacts.find(jid);
        Contact* cnt;
        if (itr != contacts.end()) {
            cnt = itr->second;
        } else {
            cnt = new Contact(jid, name);
            contacts.insert(std::make_pair(jid, cnt));
            outOfRosterContacts.insert(jid);
            cnt->setSubscriptionState(Shared::unknown);
            emit addContact(jid, "", QMap<QString, QVariant>({
                {"state", Shared::unknown}
            }));
            handleNewContact(cnt);
        }
        cnt->appendMessageToArchive(sMsg);
        
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

void Core::Account::initializeMessage(Shared::Message& target, const QXmppMessage& source, bool outgoing, bool forwarded, bool guessing) const
{
    const QDateTime& time(source.stamp());
    target.setId(source.id());
    target.setFrom(source.from());
    target.setTo(source.to());
    target.setBody(source.body());
    target.setForwarded(forwarded);
    if (guessing) {
        if (target.getFromJid() == getLogin() + "@" + getServer()) {
            outgoing = true;
        } else {
            outgoing = false;
        }
    }
    target.setOutgoing(outgoing);
    if (time.isValid()) {
        target.setTime(time);
    } else {
        target.setCurrentTime();
    }
}

void Core::Account::onMamMessageReceived(const QString& queryId, const QXmppMessage& msg)
{
    if (msg.id().size() > 0 && msg.body().size() > 0) {
        std::map<QString, QString>::const_iterator itr = achiveQueries.find(queryId);
        QString jid = itr->second;
        RosterItem* item = 0;
        std::map<QString, Contact*>::const_iterator citr = contacts.find(jid);

        if (citr != contacts.end()) {
            item = citr->second;
        } else {
            std::map<QString, Conference*>::const_iterator coitr = conferences.find(jid);
            if (coitr != conferences.end()) {
                item = coitr->second;
            }
        }
        
        Shared::Message sMsg(Shared::Message::chat);
        initializeMessage(sMsg, msg, false, true, true);
        
        item->addMessageToArchive(sMsg);
    } 
    
    //handleChatMessage(msg, false, true, true);
}

void Core::Account::requestArchive(const QString& jid, int count, const QString& before)
{
    qDebug() << "An archive request for " << jid << ", before " << before;
    RosterItem* contact = 0;
    std::map<QString, Contact*>::const_iterator itr = contacts.find(jid);
    if (itr != contacts.end()) {
        contact = itr->second;
    } else {
        std::map<QString, Conference*>::const_iterator citr = conferences.find(jid);
        if (citr != conferences.end()) {
            contact = citr->second;
        }
    }
    
    if (contact == 0) {
        qDebug() << "An attempt to request archive for" << jid << "in account" << name << ", but the contact with such id wasn't found, skipping";
        return;
    }
    
    if (contact->getArchiveState() == RosterItem::empty && before.size() == 0) {
        QXmppMessage msg(getFullJid(), jid, "", "");
        QString last = Shared::generateUUID();
        msg.setId(last);
        msg.setType(QXmppMessage::Chat);
        msg.setState(QXmppMessage::Active);
        client.sendPacket(msg);
        QTimer* timer = new QTimer;
        QObject::connect(timer, &QTimer::timeout, [timer, contact, count, last](){
            contact->requestFromEmpty(count, last);
            timer->deleteLater();
        });
        
        timer->setSingleShot(true);
        timer->start(1000);
    } else {
        contact->requestHistory(count, before);
    }
}

void Core::Account::onContactNeedHistory(const QString& before, const QString& after, const QDateTime& at)
{
    RosterItem* contact = static_cast<RosterItem*>(sender());
    QXmppResultSetQuery query;
    query.setMax(100);
    if (before.size() > 0) {
        query.setBefore(before);
    }
    QDateTime start;
    if (after.size() > 0) {     //there is some strange behavior of ejabberd server returning empty result set
        if (at.isValid()) {     //there can be some useful information about it here https://github.com/processone/ejabberd/issues/2924
            start = at;
        } else {
            query.setAfter(after);
        }
    }
    
    qDebug() << "Remote query from" << after << ", to" << before;
    
    QString q = am->retrieveArchivedMessages("", "", contact->jid, start, QDateTime(), query);
    achiveQueries.insert(std::make_pair(q, contact->jid));
}


void Core::Account::onMamResultsReceived(const QString& queryId, const QXmppResultSetReply& resultSetReply, bool complete)
{
    std::map<QString, QString>::const_iterator itr = achiveQueries.find(queryId);
    QString jid = itr->second;
    RosterItem* ri = 0;
    
    achiveQueries.erase(itr);
    std::map<QString, Contact*>::const_iterator citr = contacts.find(jid);
    if (citr != contacts.end()) {
        ri = citr->second;
    } else {
        std::map<QString, Conference*>::const_iterator coitr = conferences.find(jid);
        if (coitr != conferences.end()) {
            ri = coitr->second;
        }
    }
    
    if (ri != 0) {
        qDebug() << "Flushing messages for" << jid;
        ri->flushMessagesToArchive(complete, resultSetReply.first(), resultSetReply.last());
    }
}

void Core::Account::onMamLog(QXmppLogger::MessageType type, const QString& msg)
{
    qDebug() << "MAM MESSAGE LOG::";
    qDebug() << msg;
}

void Core::Account::onContactGroupAdded(const QString& group)
{
    Contact* contact = static_cast<Contact*>(sender());
    if (contact->groupsCount() == 1) {
        // not sure i need to handle it here, the situation with grouped and ungrouped contacts handled on the client anyway
    }
    
    QMap<QString, QVariant> cData({
        {"name", contact->getName()},
        {"state", contact->getSubscriptionState()}
    });
    addToGroup(contact->jid, group);
    emit addContact(contact->jid, group, cData);
}

void Core::Account::onContactGroupRemoved(const QString& group)
{
    Contact* contact = static_cast<Contact*>(sender());
    if (contact->groupsCount() == 0) {
        // not sure i need to handle it here, the situation with grouped and ungrouped contacts handled on the client anyway
    }
    
    emit removeContact(contact->jid, group);
    removeFromGroup(contact->jid, group);
}

void Core::Account::onContactNameChanged(const QString& cname)
{
    Contact* contact = static_cast<Contact*>(sender());
    QMap<QString, QVariant> cData({
        {"name", cname},
    });
    emit changeContact(contact->jid, cData);
}

void Core::Account::onContactSubscriptionStateChanged(Shared::SubscriptionState cstate)
{
    Contact* contact = static_cast<Contact*>(sender());
    QMap<QString, QVariant> cData({
        {"state", cstate},
    });
    emit changeContact(contact->jid, cData);
}

void Core::Account::addToGroup(const QString& jid, const QString& group)
{
    std::map<QString, std::set<QString>>::iterator gItr = groups.find(group);
    if (gItr == groups.end()) {
        gItr = groups.insert(std::make_pair(group, std::set<QString>())).first;
        emit addGroup(group);
    }
    gItr->second.insert(jid);
}

void Core::Account::removeFromGroup(const QString& jid, const QString& group)
{
    QSet<QString> toRemove;
    std::map<QString, std::set<QString>>::iterator itr = groups.find(group);
    if (itr == groups.end()) {
        qDebug() << "An attempt to remove contact" << jid << "of account" << name << "from non existing group" << group << ", skipping";
        return;
    }
    std::set<QString> contacts = itr->second;
    std::set<QString>::const_iterator cItr = contacts.find(jid);
    if (cItr != contacts.end()) {
        contacts.erase(cItr);
        if (contacts.size() == 0) {
            emit removeGroup(group);
            groups.erase(group);
        }
    }
}

Shared::SubscriptionState Core::Account::castSubscriptionState(QXmppRosterIq::Item::SubscriptionType qs) const
{
    Shared::SubscriptionState state;
    if (qs == QXmppRosterIq::Item::NotSet) {
        state = Shared::unknown;
    } else {
        state = static_cast<Shared::SubscriptionState>(qs);
    }
    return state;
}

void Core::Account::onContactHistoryResponse(const std::list<Shared::Message>& list)
{
    RosterItem* contact = static_cast<RosterItem*>(sender());
    
    qDebug() << "Collected history for contact " << contact->jid << list.size() << "elements";
    emit responseArchive(contact->jid, list);
}

void Core::Account::onClientError(QXmppClient::Error err)
{
    QString errorText;
    QString errorType;
    switch (err) {
        case QXmppClient::SocketError:
            errorText = client.socketErrorString();
            errorType = "Client socket error";
            break;
        case QXmppClient::XmppStreamError: {
            QXmppStanza::Error::Condition cnd = client.xmppStreamError();
            
            switch (cnd) {
                case QXmppStanza::Error::BadRequest:
                    errorText = "Bad request";
                    break;
                case QXmppStanza::Error::Conflict:
                    errorText = "Conflict";
                    break;
                case QXmppStanza::Error::FeatureNotImplemented:
                    errorText = "Feature is not implemented";
                    break;
                case QXmppStanza::Error::Forbidden:
                    errorText = "Forbidden";
                    break;
                case QXmppStanza::Error::Gone:
                    errorText = "Gone";
                    break;
                case QXmppStanza::Error::InternalServerError:
                    errorText = "Internal server error";
                    break;
                case QXmppStanza::Error::ItemNotFound:
                    errorText = "Item was not found";
                    break;
                case QXmppStanza::Error::JidMalformed:
                    errorText = "Malformed JID";
                    break;
                case QXmppStanza::Error::NotAcceptable:
                    errorText = "Not acceptable";
                    break;
                case QXmppStanza::Error::NotAllowed:
                    errorText = "Not allowed";
                    break;
                case QXmppStanza::Error::NotAuthorized:
                    errorText = "Authentication error";
                    break;
                case QXmppStanza::Error::PaymentRequired:
                    errorText = "Payment is required";
                    break;
                case QXmppStanza::Error::RecipientUnavailable:
                    errorText = "Recipient is unavailable";
                    break;
                case QXmppStanza::Error::Redirect:
                    errorText = "Redirected";
                    break;
                case QXmppStanza::Error::RegistrationRequired:
                    errorText = "Registration is required";
                    break;
                case QXmppStanza::Error::RemoteServerNotFound:
                    errorText = "Remote server was not found";
                    break;
                case QXmppStanza::Error::RemoteServerTimeout:
                    errorText = "Remote server timeout";
                    break;
                case QXmppStanza::Error::ResourceConstraint:
                    errorText = "Resource constraint";
                    break;
                case QXmppStanza::Error::ServiceUnavailable:
                    errorText = "Redirected";
                    break;
                case QXmppStanza::Error::SubscriptionRequired:
                    errorText = "Subscription is required";
                    break;
                case QXmppStanza::Error::UndefinedCondition:
                    errorText = "Undefined condition";
                    break;
                case QXmppStanza::Error::UnexpectedRequest:
                    errorText = "Unexpected request";
                    break;
            }
         
            errorType = "Client stream error";
        }
            
            break;
        case QXmppClient::KeepAliveError:
            errorText = "Client keep alive error";
            break;
    }
    
    qDebug() << errorType << errorText;
    emit error(errorText);
}


void Core::Account::subscribeToContact(const QString& jid, const QString& reason)
{
    if (state == Shared::connected) {
        QXmppRosterManager& rm = client.rosterManager();
        rm.subscribe(jid, reason);
    } else {
        qDebug() << "An attempt to subscribe account " << name << " to contact " << jid << " but the account is not in the connected state, skipping";
    }
}

void Core::Account::unsubscribeFromContact(const QString& jid, const QString& reason)
{
    if (state == Shared::connected) {
        QXmppRosterManager& rm = client.rosterManager();
        rm.unsubscribe(jid, reason);
    } else {
        qDebug() << "An attempt to unsubscribe account " << name << " from contact " << jid << " but the account is not in the connected state, skipping";
    }
}

void Core::Account::removeContactRequest(const QString& jid)
{
    if (state == Shared::connected) {
        std::set<QString>::const_iterator itr = outOfRosterContacts.find(jid);
        if (itr != outOfRosterContacts.end()) {
            outOfRosterContacts.erase(itr);
            onRosterItemRemoved(jid);
        } else {
            QXmppRosterManager& rm = client.rosterManager();
            rm.removeItem(jid);
        }
    } else {
        qDebug() << "An attempt to remove contact " << jid << " from account " << name << " but the account is not in the connected state, skipping";
    }
}


void Core::Account::addContactRequest(const QString& jid, const QString& name, const QSet<QString>& groups)
{
    if (state == Shared::connected) {
        std::map<QString, QString>::const_iterator itr = queuedContacts.find(jid);
        if (itr != queuedContacts.end()) {
            qDebug() << "An attempt to add contact " << jid << " to account " << name << " but the account is already queued for adding, skipping";
        } else {
            queuedContacts.insert(std::make_pair(jid, ""));     //TODO need to add reason here;
            QXmppRosterManager& rm = client.rosterManager();
            rm.addItem(jid, name, groups);
        }
    } else {
        qDebug() << "An attempt to add contact " << jid << " to account " << name << " but the account is not in the connected state, skipping";
    }
}

void Core::Account::onMucRoomAdded(QXmppMucRoom* room)
{
    qDebug() << "room" << room->jid() << "added with name" << room->name() << ", account" << getName() << "joined:" << room->isJoined(); 
}

void Core::Account::bookmarksReceived(const QXmppBookmarkSet& bookmarks)
{
    QList<QXmppBookmarkConference> confs = bookmarks.conferences();
    for (QList<QXmppBookmarkConference>::const_iterator itr = confs.begin(), end = confs.end(); itr != end; ++itr) {
        const QXmppBookmarkConference& c = *itr;
        
        QString jid = c.jid();
        std::map<QString, Conference*>::const_iterator cItr = conferences.find(jid);
        if (cItr == conferences.end()) {
            QXmppMucRoom* room = mm->addRoom(jid);
            QString nick = c.nickName();
            Conference* conf = new Conference(jid, getName(), c.autoJoin(), c.name(), nick == "" ? getName() : nick, room);
            
            handleNewConference(conf);
            
            emit addRoom(jid, {
                {"autoJoin", conf->getAutoJoin()},
                {"joined", conf->getJoined()},
                {"nick", conf->getNick()},
                {"name", conf->getName()}
            });
        } else {
            qDebug() << "Received a bookmark to a MUC " << jid << " which is already booked by another bookmark, skipping";
        }
    }
}

void Core::Account::onMucJoinedChanged(bool joined)
{
    Conference* room = static_cast<Conference*>(sender());
    emit changeRoom(room->jid, {
            {"joined", joined}
        });
}

void Core::Account::onMucAutoJoinChanged(bool autoJoin)
{
    Conference* room = static_cast<Conference*>(sender());
    emit changeRoom(room->jid, {
            {"autoJoin", autoJoin}
        });
}

void Core::Account::onMucNickNameChanged(const QString& nickName)
{
    Conference* room = static_cast<Conference*>(sender());
    emit changeRoom(room->jid, {
            {"nick", nickName}
        });
}
