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

using namespace Core;

Account::Account(const QString& p_login, const QString& p_server, const QString& p_password, const QString& p_name, NetworkAccess* p_net, QObject* parent):
    QObject(parent),
    name(p_name),
    achiveQueries(),
    client(),
    config(),
    presence(),
    state(Shared::ConnectionState::disconnected),
    groups(),
    cm(new QXmppCarbonManager()),
    am(new QXmppMamManager()),
    mm(new QXmppMucManager()),
    bm(new QXmppBookmarkManager()),
    rm(client.findExtension<QXmppRosterManager>()),
    vm(client.findExtension<QXmppVCardManager>()),
    um(new QXmppUploadRequestManager()),
    dm(client.findExtension<QXmppDiscoveryManager>()),
    rcpm(new QXmppMessageReceiptManager()),
    contacts(),
    conferences(),
    reconnectScheduled(false),
    reconnectTimer(new QTimer),
    queuedContacts(),
    outOfRosterContacts(),
    pendingMessages(),
    uploadingSlotsQueue(),
    avatarHash(),
    avatarType(),
    ownVCardRequestInProgress(false),
    network(p_net),
    passwordType(Shared::AccountPassword::plain),
    mh(new MessageHandler(this))
{
    config.setUser(p_login);
    config.setDomain(p_server);
    config.setPassword(p_password);
    config.setAutoAcceptSubscriptions(true);
    //config.setAutoReconnectionEnabled(false);
    
    QObject::connect(&client, &QXmppClient::stateChanged, this, &Account::onClientStateChange);
    QObject::connect(&client, &QXmppClient::presenceReceived, this, &Account::onPresenceReceived);
    QObject::connect(&client, &QXmppClient::messageReceived, mh, &MessageHandler::onMessageReceived);
    QObject::connect(&client, &QXmppClient::error, this, &Account::onClientError);
    
    QObject::connect(rm, &QXmppRosterManager::rosterReceived, this, &Account::onRosterReceived);
    QObject::connect(rm, &QXmppRosterManager::itemAdded, this, &Account::onRosterItemAdded);
    QObject::connect(rm, &QXmppRosterManager::itemRemoved, this, &Account::onRosterItemRemoved);
    QObject::connect(rm, &QXmppRosterManager::itemChanged, this, &Account::onRosterItemChanged);
    //QObject::connect(&rm, &QXmppRosterManager::presenceChanged, this, &Account::onRosterPresenceChanged);
    
    client.addExtension(cm);
    
    QObject::connect(cm, &QXmppCarbonManager::messageReceived, mh, &MessageHandler::onCarbonMessageReceived);
    QObject::connect(cm, &QXmppCarbonManager::messageSent, mh, &MessageHandler::onCarbonMessageSent);
    
    client.addExtension(am);
    
    QObject::connect(am, &QXmppMamManager::logMessage, this, &Account::onMamLog);
    QObject::connect(am, &QXmppMamManager::archivedMessageReceived, this, &Account::onMamMessageReceived);
    QObject::connect(am, &QXmppMamManager::resultsRecieved, this, &Account::onMamResultsReceived);
    
    client.addExtension(mm);
    QObject::connect(mm, &QXmppMucManager::roomAdded, this, &Account::onMucRoomAdded);
    
    client.addExtension(bm);
    QObject::connect(bm, &QXmppBookmarkManager::bookmarksReceived, this, &Account::bookmarksReceived);
    
    QObject::connect(vm, &QXmppVCardManager::vCardReceived, this, &Account::onVCardReceived);
    //QObject::connect(&vm, &QXmppVCardManager::clientVCardReceived, this, &Account::onOwnVCardReceived); //for some reason it doesn't work, launching from common handler
    
    client.addExtension(um);
    QObject::connect(um, &QXmppUploadRequestManager::slotReceived, mh, &MessageHandler::onUploadSlotReceived);
    QObject::connect(um, &QXmppUploadRequestManager::requestFailed, mh, &MessageHandler::onUploadSlotRequestFailed);
    
    QObject::connect(dm, &QXmppDiscoveryManager::itemsReceived, this, &Account::onDiscoveryItemsReceived);
    QObject::connect(dm, &QXmppDiscoveryManager::infoReceived, this, &Account::onDiscoveryInfoReceived);
    
    QObject::connect(network, &NetworkAccess::uploadFileComplete, mh, &MessageHandler::onFileUploaded);
    QObject::connect(network, &NetworkAccess::uploadFileError, mh, &MessageHandler::onFileUploadError);
    
    client.addExtension(rcpm);
    QObject::connect(rcpm, &QXmppMessageReceiptManager::messageDelivered, mh, &MessageHandler::onReceiptReceived);
    
    
    QString path(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
    path += "/" + name;
    QDir dir(path);
    
    if (!dir.exists()) {
        bool res = dir.mkpath(path);
        if (!res) {
            qDebug() << "Couldn't create a cache directory for account" << name;
            throw 22;
        }
    }
    
    QFile* avatar = new QFile(path + "/avatar.png");
    QString type = "png";
    if (!avatar->exists()) {
        delete avatar;
        avatar = new QFile(path + "/avatar.jpg");
        type = "jpg";
        if (!avatar->exists()) {
            delete avatar;
            avatar = new QFile(path + "/avatar.jpeg");
            type = "jpeg";
            if (!avatar->exists()) {
                delete avatar;
                avatar = new QFile(path + "/avatar.gif");
                type = "gif";
            }
        }
    }
    
    if (avatar->exists()) {
        if (avatar->open(QFile::ReadOnly)) {
            QCryptographicHash sha1(QCryptographicHash::Sha1);
            sha1.addData(avatar);
            avatarHash = sha1.result();
            avatarType = type;
        }
    }
    if (avatarType.size() != 0) {
        presence.setVCardUpdateType(QXmppPresence::VCardUpdateValidPhoto);
        presence.setPhotoHash(avatarHash.toUtf8());
    } else {
        presence.setVCardUpdateType(QXmppPresence::VCardUpdateNotReady);
    }
    
    reconnectTimer->setSingleShot(true);
    QObject::connect(reconnectTimer, &QTimer::timeout, this, &Account::connect);
    
//     QXmppLogger* logger = new QXmppLogger(this);
//     logger->setLoggingType(QXmppLogger::SignalLogging);
//     client.setLogger(logger);
//     
//     QObject::connect(logger, &QXmppLogger::message, this, [](QXmppLogger::MessageType type, const QString& text){
//         qDebug() << text;
//     });
}

Account::~Account()
{
    if (reconnectScheduled) {
        reconnectScheduled = false;
        reconnectTimer->stop();
    }
    
    QObject::disconnect(network, &NetworkAccess::uploadFileComplete, mh, &MessageHandler::onFileUploaded);
    QObject::disconnect(network, &NetworkAccess::uploadFileError, mh, &MessageHandler::onFileUploadError);
    
    for (std::map<QString, Contact*>::const_iterator itr = contacts.begin(), end = contacts.end(); itr != end; ++itr) {
        delete itr->second;
    }
    
    for (std::map<QString, Conference*>::const_iterator itr = conferences.begin(), end = conferences.end(); itr != end; ++itr) {
        delete itr->second;
    }
    
    delete reconnectTimer;
    delete rcpm;
    delete dm;
    delete um;
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
    if (state == Shared::ConnectionState::disconnected) {
        qDebug() << presence.availableStatusType();
        client.connectToServer(config, presence);
    } else {
        qDebug("An attempt to connect an account which is already connected, skipping");
    }
}

void Core::Account::disconnect()
{
    if (reconnectScheduled) {
        reconnectScheduled = false;
        reconnectTimer->stop();
    }
    if (state != Shared::ConnectionState::disconnected) {
        clearConferences();
        client.disconnectFromServer();
    }
}

void Core::Account::onClientStateChange(QXmppClient::State st)
{
    switch (st) {
        case QXmppClient::ConnectedState: {
            if (state != Shared::ConnectionState::connected) {
                if (client.isActive()) {
                    Shared::ConnectionState os = state;
                    state = Shared::ConnectionState::connected;
                    if (os == Shared::ConnectionState::connecting) {
                        dm->requestItems(getServer());
                        dm->requestInfo(getServer());
                    }
                    emit connectionStateChanged(state);
                }
            } else {
                qDebug()    << "Something weird happened - xmpp client of account" << name
                            << "reported about successful connection but account was in" << state << "state";
            }
        }
            break;
        case QXmppClient::ConnectingState: {
            if (state != Shared::ConnectionState::connecting) {
                state = Shared::ConnectionState::connecting;
                emit connectionStateChanged(state);
            }
        }
            break;
        case QXmppClient::DisconnectedState: {
            cancelHistoryRequests();
            pendingVCardRequests.clear();
            if (state != Shared::ConnectionState::disconnected) {
                state = Shared::ConnectionState::disconnected;
                emit connectionStateChanged(state);
            } else {
                qDebug()    << "Something weird happened - xmpp client of account" << name
                            << "reported about disconnection but account was in" << state << "state";
            }
        }
            break;
    }
}

void Core::Account::reconnect()
{
    if (state == Shared::ConnectionState::connected && !reconnectScheduled) {
        reconnectScheduled = true;
        reconnectTimer->start(500);
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

Shared::AccountPassword Core::Account::getPasswordType() const
{
    return passwordType;
}

void Core::Account::onRosterReceived()
{
    vm->requestClientVCard();         //TODO need to make sure server actually supports vCards
    ownVCardRequestInProgress = true;
    
    QStringList bj = rm->getRosterBareJids();
    for (int i = 0; i < bj.size(); ++i) {
        const QString& jid = bj[i];
        addedAccount(jid);
    }
}

void Core::Account::onRosterItemAdded(const QString& bareJid)
{
    addedAccount(bareJid);
    std::map<QString, QString>::const_iterator itr = queuedContacts.find(bareJid);
    if (itr != queuedContacts.end()) {
        rm->subscribe(bareJid, itr->second);
        queuedContacts.erase(itr);
    }
}

void Core::Account::setPasswordType(Shared::AccountPassword pt)
{
    passwordType = pt;
}

void Core::Account::onRosterItemChanged(const QString& bareJid)
{
    std::map<QString, Contact*>::const_iterator itr = contacts.find(bareJid);
    if (itr == contacts.end()) {
        qDebug() << "An attempt to change non existing contact" << bareJid << "from account" << name << ", skipping";
        return;
    }
    Contact* contact = itr->second;
    QXmppRosterIq::Item re = rm->getRosterEntry(bareJid);
    
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
    std::map<QString, Contact*>::const_iterator itr = contacts.find(jid);
    QXmppRosterIq::Item re = rm->getRosterEntry(jid);
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
            {"state", QVariant::fromValue(state)}
        });
        
        Archive::AvatarInfo info;
        bool hasAvatar = contact->readAvatarInfo(info);
        if (hasAvatar) {
            if (info.autogenerated) {
                cData.insert("avatarState", static_cast<uint>(Shared::Avatar::valid));
            } else {
                cData.insert("avatarState", static_cast<uint>(Shared::Avatar::autocreated));
            }
            cData.insert("avatarPath", contact->avatarPath() + "." + info.type);
        } else {
            cData.insert("avatarState", static_cast<uint>(Shared::Avatar::empty));
            cData.insert("avatarPath", "");
            requestVCard(jid);
        }
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
    QObject::connect(contact, &RosterItem::needHistory, this, &Account::onContactNeedHistory);
    QObject::connect(contact, &RosterItem::historyResponse, this, &Account::onContactHistoryResponse);
    QObject::connect(contact, &RosterItem::nameChanged, this, &Account::onContactNameChanged);
    QObject::connect(contact, &RosterItem::avatarChanged, this, &Account::onContactAvatarChanged);
    QObject::connect(contact, &RosterItem::requestVCard, this, &Account::requestVCard);
}

void Core::Account::handleNewContact(Core::Contact* contact)
{
    handleNewRosterItem(contact);
    QObject::connect(contact, &Contact::groupAdded, this, &Account::onContactGroupAdded);
    QObject::connect(contact, &Contact::groupRemoved, this, &Account::onContactGroupRemoved);
    QObject::connect(contact, &Contact::subscriptionStateChanged, this, &Account::onContactSubscriptionStateChanged);
}

void Core::Account::handleNewConference(Core::Conference* contact)
{
    handleNewRosterItem(contact);
    QObject::connect(contact, &Conference::nickChanged, this, &Account::onMucNickNameChanged);
    QObject::connect(contact, &Conference::subjectChanged, this, &Account::onMucSubjectChanged);
    QObject::connect(contact, &Conference::joinedChanged, this, &Account::onMucJoinedChanged);
    QObject::connect(contact, &Conference::autoJoinChanged, this, &Account::onMucAutoJoinChanged);
    QObject::connect(contact, &Conference::addParticipant, this, &Account::onMucAddParticipant);
    QObject::connect(contact, &Conference::changeParticipant, this, &Account::onMucChangeParticipant);
    QObject::connect(contact, &Conference::removeParticipant, this, &Account::onMucRemoveParticipant);
}

void Core::Account::onPresenceReceived(const QXmppPresence& p_presence)
{
    QString id = p_presence.from();
    QStringList comps = id.split("/");
    QString jid = comps.front();
    QString resource = comps.back();
    
    QString myJid = getLogin() + "@" + getServer();
    
    if (jid == myJid) {
        if (resource == getResource()) {
            emit availabilityChanged(static_cast<Shared::Availability>(p_presence.availableStatusType()));
        } else {
            if (!ownVCardRequestInProgress) {
                switch (p_presence.vCardUpdateType()) {
                    case QXmppPresence::VCardUpdateNone:            //this presence has nothing to do with photo
                        break;
                    case QXmppPresence::VCardUpdateNotReady:        //let's say the photo didn't change here
                        break;
                    case QXmppPresence::VCardUpdateNoPhoto:         //there is no photo, need to drop if any
                        if (avatarType.size() > 0) {
                            vm->requestClientVCard();
                            ownVCardRequestInProgress = true;
                        }
                        break;
                    case QXmppPresence::VCardUpdateValidPhoto:      //there is a photo, need to load
                        if (avatarHash != p_presence.photoHash()) {
                            vm->requestClientVCard();
                            ownVCardRequestInProgress = true;
                        }
                        break;
                }
            }
        }
    } else {
        if (pendingVCardRequests.find(jid) == pendingVCardRequests.end()) {
            std::map<QString, Contact*>::const_iterator itr = contacts.find(jid);
            if (itr != contacts.end()) {
                itr->second->handlePresence(p_presence);
            }
        }
    }
    
    switch (p_presence.type()) {
        case QXmppPresence::Error:
            qDebug() << "An error reported by presence from" << id << p_presence.error().text();
            break;
        case QXmppPresence::Available:{
            QDateTime lastInteraction = p_presence.lastUserInteraction();
            if (!lastInteraction.isValid()) {
                lastInteraction = QDateTime::currentDateTimeUtc();
            }
            emit addPresence(jid, resource, {
                {"lastActivity", lastInteraction},
                {"availability", p_presence.availableStatusType()},           //TODO check and handle invisible
                {"status", p_presence.statusText()}
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
    const QXmppPresence& presence = rm->getPresence(bareJid, resource);
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
    if (state == Shared::ConnectionState::connected) {
        QXmppPresence::AvailableStatusType pres = presence.availableStatusType();
        return static_cast<Shared::Availability>(pres);         //they are compatible;
    } else {
        return Shared::Availability::offline;
    }
}

void Core::Account::setAvailability(Shared::Availability avail)
{
    if (avail == Shared::Availability::offline) {
        disconnect();               //TODO not sure how to do here - changing state may cause connection or disconnection
    } else {
        QXmppPresence::AvailableStatusType pres = static_cast<QXmppPresence::AvailableStatusType>(avail);
        
        presence.setAvailableStatusType(pres);
        if (state != Shared::ConnectionState::disconnected) {
            client.setClientPresence(presence);
        }
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

QString Core::Account::getFullJid() const
{
    return getLogin() + "@" + getServer() + "/" + getResource();
}

void Core::Account::sendMessage(const Shared::Message& data)
{
    mh->sendMessage(data);
}

void Core::Account::sendMessage(const Shared::Message& data, const QString& path)
{
    mh->sendMessage(data, path);
}

void Core::Account::onMamMessageReceived(const QString& queryId, const QXmppMessage& msg)
{
    if (msg.id().size() > 0 && (msg.body().size() > 0 || msg.outOfBandUrl().size() > 0)) {
        std::map<QString, QString>::const_iterator itr = achiveQueries.find(queryId);
        if (itr != achiveQueries.end()) {
            QString jid = itr->second;
            RosterItem* item = getRosterItem(jid);
            
            Shared::Message sMsg(static_cast<Shared::Message::Type>(msg.type()));
            mh->initializeMessage(sMsg, msg, false, true, true);
            sMsg.setState(Shared::Message::State::sent);
            
            QString oId = msg.replaceId();
            if (oId.size() > 0) {
                item->correctMessageInArchive(oId, sMsg);
            } else {
                item->addMessageToArchive(sMsg);
            }
        }
    } 
    
    //handleChatMessage(msg, false, true, true);
}

Core::RosterItem * Core::Account::getRosterItem(const QString& jid)
{
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
    
    return item;
}

void Core::Account::requestArchive(const QString& jid, int count, const QString& before)
{
    qDebug() << "An archive request for " << jid << ", before " << before;
    RosterItem* contact = getRosterItem(jid);
    
    if (contact == 0) {
        qDebug() << "An attempt to request archive for" << jid << "in account" << name << ", but the contact with such id wasn't found, skipping";
        emit responseArchive(jid, std::list<Shared::Message>());
        return;
    }
    
    if (state != Shared::ConnectionState::connected) {
        qDebug() << "An attempt to request archive for" << jid << "in account" << name << ", but the account is not online, skipping";
        emit responseArchive(contact->jid, std::list<Shared::Message>());
    }
    
    contact->requestHistory(count, before);
}

void Core::Account::onContactNeedHistory(const QString& before, const QString& after, const QDateTime& at)
{
    RosterItem* contact = static_cast<RosterItem*>(sender());

    QString to;
    QString with;
    QXmppResultSetQuery query;
    QDateTime start;
    query.setMax(100);
    
    if (contact->getArchiveState() == RosterItem::empty) {
        query.setBefore(before);
        qDebug() << "Requesting remote history from empty for" << contact->jid;
    } else {
        if (before.size() > 0) {
            query.setBefore(before);
        }
        if (after.size() > 0) {     //there is some strange behavior of ejabberd server returning empty result set
            if (at.isValid()) {     //there can be some useful information about it here https://github.com/processone/ejabberd/issues/2924
                start = at;
            } else {
                query.setAfter(after);
            }
        }
        qDebug() << "Remote query for" << contact->jid << "from" << after << ", to" << before;
    }
    
    if (contact->isMuc()) {
        to = contact->jid;
    } else {
        with = contact->jid;
    }
    
    
    QString q = am->retrieveArchivedMessages(to, "", with, start, QDateTime(), query);
    achiveQueries.insert(std::make_pair(q, contact->jid));
}


void Core::Account::onMamResultsReceived(const QString& queryId, const QXmppResultSetReply& resultSetReply, bool complete)
{
    std::map<QString, QString>::const_iterator itr = achiveQueries.find(queryId);
    if (itr != achiveQueries.end()) {
        QString jid = itr->second;
        achiveQueries.erase(itr);
        
        RosterItem* ri = getRosterItem(jid);
        
        if (ri != 0) {
            qDebug() << "Flushing messages for" << jid << ", complete:" << complete;
            ri->flushMessagesToArchive(complete, resultSetReply.first(), resultSetReply.last());
        }
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
        {"state", QVariant::fromValue(contact->getSubscriptionState())}
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
        {"state", QVariant::fromValue(cstate)},
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
        state = Shared::SubscriptionState::unknown;
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
    qDebug() << "Error";
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
#if (QXMPP_VERSION) >= QT_VERSION_CHECK(1, 3, 0)
                case QXmppStanza::Error::PolicyViolation:
                    errorText = "Policy violation";
                    break;
#endif
            }
         
            errorType = "Client stream error";
        }
            
            break;
        case QXmppClient::KeepAliveError:
            errorText = "Client keep alive error";
            break;
        case QXmppClient::NoError:
            break;                      //not exactly sure what to do here
    }
    
    qDebug() << errorType << errorText;
    emit error(errorText);
}


void Core::Account::subscribeToContact(const QString& jid, const QString& reason)
{
    if (state == Shared::ConnectionState::connected) {
        rm->subscribe(jid, reason);
    } else {
        qDebug() << "An attempt to subscribe account " << name << " to contact " << jid << " but the account is not in the connected state, skipping";
    }
}

void Core::Account::unsubscribeFromContact(const QString& jid, const QString& reason)
{
    if (state == Shared::ConnectionState::connected) {
        rm->unsubscribe(jid, reason);
    } else {
        qDebug() << "An attempt to unsubscribe account " << name << " from contact " << jid << " but the account is not in the connected state, skipping";
    }
}

void Core::Account::removeContactRequest(const QString& jid)
{
    if (state == Shared::ConnectionState::connected) {
        std::set<QString>::const_iterator itr = outOfRosterContacts.find(jid);
        if (itr != outOfRosterContacts.end()) {
            outOfRosterContacts.erase(itr);
            onRosterItemRemoved(jid);
        } else {
            rm->removeItem(jid);
        }
    } else {
        qDebug() << "An attempt to remove contact " << jid << " from account " << name << " but the account is not in the connected state, skipping";
    }
}


void Core::Account::addContactRequest(const QString& jid, const QString& name, const QSet<QString>& groups)
{
    if (state == Shared::ConnectionState::connected) {
        std::map<QString, QString>::const_iterator itr = queuedContacts.find(jid);
        if (itr != queuedContacts.end()) {
            qDebug() << "An attempt to add contact " << jid << " to account " << name << " but the account is already queued for adding, skipping";
        } else {
            queuedContacts.insert(std::make_pair(jid, ""));     //TODO need to add reason here;
            rm->addItem(jid, name, groups);
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
            addNewRoom(jid, c.nickName(), c.name(), c.autoJoin());
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
    storeConferences();
    Conference* room = static_cast<Conference*>(sender());
    emit changeRoom(room->jid, {
            {"autoJoin", autoJoin}
        });
}

void Core::Account::onMucNickNameChanged(const QString& nickName)
{
    storeConferences();
    Conference* room = static_cast<Conference*>(sender());
    emit changeRoom(room->jid, {
            {"nick", nickName}
        });
}

void Core::Account::setRoomAutoJoin(const QString& jid, bool joined)
{
    std::map<QString, Conference*>::const_iterator cItr = conferences.find(jid);
    if (cItr == conferences.end()) {
        qDebug() << "An attempt to set auto join to the non existing room" << jid << "of the account" << getName() << ", skipping";
        return;
    }
    
    cItr->second->setAutoJoin(joined);
}

void Core::Account::setRoomJoined(const QString& jid, bool joined)
{
    std::map<QString, Conference*>::const_iterator cItr = conferences.find(jid);
    if (cItr == conferences.end()) {
        qDebug() << "An attempt to set joined to the non existing room" << jid << "of the account" << getName() << ", skipping";
        return;
    }
    
    cItr->second->setJoined(joined);
}

void Core::Account::onMucAddParticipant(const QString& nickName, const QMap<QString, QVariant>& data)
{
    Conference* room = static_cast<Conference*>(sender());
    emit addRoomParticipant(room->jid, nickName, data);
}

void Core::Account::onMucChangeParticipant(const QString& nickName, const QMap<QString, QVariant>& data)
{
    Conference* room = static_cast<Conference*>(sender());
    emit changeRoomParticipant(room->jid, nickName, data);
}

void Core::Account::onMucRemoveParticipant(const QString& nickName)
{
    Conference* room = static_cast<Conference*>(sender());
    emit removeRoomParticipant(room->jid, nickName);
}

void Core::Account::onMucSubjectChanged(const QString& subject)
{
    Conference* room = static_cast<Conference*>(sender());
    emit changeRoom(room->jid, {
        {"subject", subject}
    });
}

void Core::Account::storeConferences()
{
    QXmppBookmarkSet bms = bm->bookmarks();
    QList<QXmppBookmarkConference> confs;
    for (std::map<QString, Conference*>::const_iterator itr = conferences.begin(), end = conferences.end(); itr != end; ++itr) {
        Conference* conference = itr->second;
        QXmppBookmarkConference conf;
        conf.setJid(conference->jid);
        conf.setName(conference->getName());
        conf.setNickName(conference->getNick());
        conf.setAutoJoin(conference->getAutoJoin());
        confs.push_back(conf);
    }
    bms.setConferences(confs);
    bm->setBookmarks(bms);
}

void Core::Account::clearConferences()
{
    for (std::map<QString, Conference*>::const_iterator itr = conferences.begin(), end = conferences.end(); itr != end; itr++) {
        itr->second->deleteLater();
        emit removeRoom(itr->first);
    }
    conferences.clear();
}

void Core::Account::removeRoomRequest(const QString& jid)
{
    std::map<QString, Conference*>::const_iterator itr = conferences.find(jid);
    if (itr == conferences.end()) {
        qDebug() << "An attempt to remove non existing room" << jid << "from account" << name << ", skipping";
    }
    itr->second->deleteLater();
    conferences.erase(itr);
    emit removeRoom(jid);
    storeConferences();
}

void Core::Account::addRoomRequest(const QString& jid, const QString& nick, const QString& password, bool autoJoin)
{
    std::map<QString, Conference*>::const_iterator cItr = conferences.find(jid);
    if (cItr == conferences.end()) {
        addNewRoom(jid, nick, "", autoJoin);
        storeConferences();
    } else {
        qDebug() << "An attempt to add a MUC " << jid << " which is already present in the rester, skipping";
    }
}

void Core::Account::addNewRoom(const QString& jid, const QString& nick, const QString& roomName, bool autoJoin)
{
    QXmppMucRoom* room = mm->addRoom(jid);
    QString lNick = nick;
    if (lNick.size() == 0) {
        lNick = getName();
    }
    Conference* conf = new Conference(jid, getName(), autoJoin, roomName, lNick, room);
    conferences.insert(std::make_pair(jid, conf));
    
    handleNewConference(conf);
    
    QMap<QString, QVariant> cData = {
        {"autoJoin", conf->getAutoJoin()},
        {"joined", conf->getJoined()},
        {"nick", conf->getNick()},
        {"name", conf->getName()},
        {"avatars", conf->getAllAvatars()}
    };
    
    
    Archive::AvatarInfo info;
    bool hasAvatar = conf->readAvatarInfo(info);
    if (hasAvatar) {
        if (info.autogenerated) {
            cData.insert("avatarState", QVariant::fromValue(Shared::Avatar::autocreated));
        } else {
            cData.insert("avatarState", QVariant::fromValue(Shared::Avatar::valid));
        }
        cData.insert("avatarPath", conf->avatarPath() + "." + info.type);
    } else {
        cData.insert("avatarState", QVariant::fromValue(Shared::Avatar::empty));
        cData.insert("avatarPath", "");
        requestVCard(jid);
    }
    
    emit addRoom(jid, cData);
}

void Core::Account::addContactToGroupRequest(const QString& jid, const QString& groupName)
{
    std::map<QString, Contact*>::const_iterator itr = contacts.find(jid);
    if (itr == contacts.end()) {
        qDebug() << "An attempt to add non existing contact" << jid << "of account" << name << "to the group" << groupName << ", skipping";
    } else {
        QXmppRosterIq::Item item = rm->getRosterEntry(jid);
        QSet<QString> groups = item.groups();
        if (groups.find(groupName) == groups.end()) {           //TODO need to change it, I guess that sort of code is better in qxmpp lib
            groups.insert(groupName);
            item.setGroups(groups);
            
            QXmppRosterIq iq;
            iq.setType(QXmppIq::Set);
            iq.addItem(item);
            client.sendPacket(iq);
        } else {
            qDebug() << "An attempt to add contact" << jid << "of account" << name << "to the group" << groupName << "but it's already in that group, skipping";
        }
    }
}

void Core::Account::removeContactFromGroupRequest(const QString& jid, const QString& groupName)
{
    std::map<QString, Contact*>::const_iterator itr = contacts.find(jid);
    if (itr == contacts.end()) {
        qDebug() << "An attempt to remove non existing contact" << jid << "of account" << name << "from the group" << groupName << ", skipping";
    } else {
        QXmppRosterIq::Item item = rm->getRosterEntry(jid);
        QSet<QString> groups = item.groups();
        QSet<QString>::const_iterator gItr = groups.find(groupName);
        if (gItr != groups.end()) {
            groups.erase(gItr);
            item.setGroups(groups);
            
            QXmppRosterIq iq;
            iq.setType(QXmppIq::Set);
            iq.addItem(item);
            client.sendPacket(iq);
        } else {
            qDebug() << "An attempt to remove contact" << jid << "of account" << name << "from the group" << groupName << "but it's not in that group, skipping";
        }
    }
}

void Core::Account::renameContactRequest(const QString& jid, const QString& newName)
{
    std::map<QString, Contact*>::const_iterator itr = contacts.find(jid);
    if (itr == contacts.end()) {
        qDebug() << "An attempt to rename non existing contact" << jid << "of account" << name << ", skipping";
    } else {
        rm->renameItem(jid, newName);
    }
}

void Core::Account::onVCardReceived(const QXmppVCardIq& card)
{
    QString id = card.from();
    QStringList comps = id.split("/");
    QString jid = comps.front();
    QString resource("");
    if (comps.size() > 1) {
        resource = comps.back();
    }
    pendingVCardRequests.erase(id);
    RosterItem* item = getRosterItem(jid);
    
    if (item == 0) {
        if (jid == getLogin() + "@" + getServer()) {
            onOwnVCardReceived(card);
        } else {
            qDebug() << "received vCard" << jid << "doesn't belong to any of known contacts or conferences, skipping";
        }
        return;
    }
    
    Shared::VCard vCard = item->handleResponseVCard(card, resource);
    
    emit receivedVCard(jid, vCard);
}

void Core::Account::onOwnVCardReceived(const QXmppVCardIq& card)
{
    QByteArray ava = card.photo();
    bool avaChanged = false;
    QString path = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/" + name + "/";
    if (ava.size() > 0) {
        QCryptographicHash sha1(QCryptographicHash::Sha1);
        sha1.addData(ava);
        QString newHash(sha1.result());
        QMimeDatabase db;
        QMimeType newType = db.mimeTypeForData(ava);
        if (avatarType.size() > 0) {
            if (avatarHash != newHash) {
                QString oldPath = path + "avatar." + avatarType;
                QFile oldAvatar(oldPath);
                bool oldToRemove = false;
                if (oldAvatar.exists()) {
                    if (oldAvatar.rename(oldPath + ".bak")) {
                        oldToRemove = true;
                    } else {
                        qDebug() << "Received new avatar for account" << name << "but can't get rid of the old one, doing nothing";
                    }
                }
                QFile newAvatar(path + "avatar." + newType.preferredSuffix());
                if (newAvatar.open(QFile::WriteOnly)) {
                    newAvatar.write(ava);
                    newAvatar.close();
                    avatarHash = newHash;
                    avatarType = newType.preferredSuffix();
                    avaChanged = true;
                } else {
                    qDebug() << "Received new avatar for account" << name << "but can't save it";
                    if (oldToRemove) {
                        qDebug() << "rolling back to the old avatar";
                        if (!oldAvatar.rename(oldPath)) {
                            qDebug() << "Couldn't roll back to the old avatar in account" << name;
                        }
                    }
                }
            }
        } else {
            QFile newAvatar(path + "avatar." + newType.preferredSuffix());
            if (newAvatar.open(QFile::WriteOnly)) {
                newAvatar.write(ava);
                newAvatar.close();
                avatarHash = newHash;
                avatarType = newType.preferredSuffix();
                avaChanged = true;
            } else {
                qDebug() << "Received new avatar for account" << name << "but can't save it";
            }
        }
    } else {
        if (avatarType.size() > 0) {
            QFile oldAvatar(path + "avatar." + avatarType);
            if (!oldAvatar.remove()) {
                qDebug() << "Received vCard for account" << name << "without avatar, but can't get rid of the file, doing nothing";
            } else {
                avatarType = "";
                avatarHash = "";
                avaChanged = true;
            }
        }
    }
    
    if (avaChanged) {
        QMap<QString, QVariant> change;
        if (avatarType.size() > 0) {
            presence.setPhotoHash(avatarHash.toUtf8());
            presence.setVCardUpdateType(QXmppPresence::VCardUpdateValidPhoto);
            change.insert("avatarPath", path + "avatar." + avatarType);
        } else {
            presence.setPhotoHash("");
            presence.setVCardUpdateType(QXmppPresence::VCardUpdateNoPhoto);
            change.insert("avatarPath", "");
        }
        client.setClientPresence(presence);
        emit changed(change);
    }
    
    ownVCardRequestInProgress = false;
    
    Shared::VCard vCard;
    initializeVCard(vCard, card);
    
    if (avatarType.size() > 0) {
        vCard.setAvatarType(Shared::Avatar::valid);
        vCard.setAvatarPath(path + "avatar." + avatarType);
    } else {
        vCard.setAvatarType(Shared::Avatar::empty);
    }
    
    emit receivedVCard(getLogin() + "@" + getServer(), vCard);
}

QString Core::Account::getAvatarPath() const
{
    if (avatarType.size() == 0) {
        return "";
    } else {
        return QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/" + name + "/" + "avatar." + avatarType;
    }
}

void Core::Account::onContactAvatarChanged(Shared::Avatar type, const QString& path)
{
    RosterItem* item = static_cast<RosterItem*>(sender());
    QMap<QString, QVariant> cData({
        {"avatarState", static_cast<uint>(type)},
        {"avatarPath", path}
    });
    
    emit changeContact(item->jid, cData);
}

void Core::Account::requestVCard(const QString& jid)
{
    if (pendingVCardRequests.find(jid) == pendingVCardRequests.end()) {
        qDebug() << "requesting vCard" << jid;
        if (jid == getLogin() + "@" + getServer()) {
            if (!ownVCardRequestInProgress) {
                vm->requestClientVCard();
                ownVCardRequestInProgress = true;
            }
        } else {
            vm->requestVCard(jid);
            pendingVCardRequests.insert(jid);
        }
    }
}

void Core::Account::uploadVCard(const Shared::VCard& card)
{
    QXmppVCardIq iq;
    initializeQXmppVCard(iq, card);
    
    bool avatarChanged = false;
    if (card.getAvatarType() != Shared::Avatar::empty) {
        QString newPath = card.getAvatarPath();
        QString oldPath = getAvatarPath();
        QByteArray data;
        QString type;
        if (newPath != oldPath) {
            QFile avatar(newPath);
            if (!avatar.open(QFile::ReadOnly)) {
                qDebug() << "An attempt to upload new vCard to account" << name 
                << "but it wasn't possible to read file" << newPath 
                << "which was supposed to be new avatar, uploading old avatar";
                if (avatarType.size() > 0) {
                    QFile oA(oldPath);
                    if (!oA.open(QFile::ReadOnly)) {
                        qDebug() << "Couldn't read old avatar of account" << name << ", uploading empty avatar";
                    } else {
                        data = oA.readAll();
                    }
                }
            } else {
                data = avatar.readAll();
                avatarChanged = true;
            }
        } else {
            if (avatarType.size() > 0) {
                QFile oA(oldPath);
                if (!oA.open(QFile::ReadOnly)) {
                    qDebug() << "Couldn't read old avatar of account" << name << ", uploading empty avatar";
                } else {
                    data = oA.readAll();
                }
            }
        }
        
        if (data.size() > 0) {
            QMimeDatabase db;
            type = db.mimeTypeForData(data).name();
            iq.setPhoto(data);
            iq.setPhotoType(type);
        }
    }
    
    vm->setClientVCard(iq);
    onOwnVCardReceived(iq);
}

void Core::Account::onDiscoveryItemsReceived(const QXmppDiscoveryIq& items)
{
    for (QXmppDiscoveryIq::Item item : items.items()) {
        if (item.jid() != getServer()) {
            dm->requestInfo(item.jid());
        }
    }
}

void Core::Account::onDiscoveryInfoReceived(const QXmppDiscoveryIq& info)
{
    if (info.from() == getServer()) {
        if (info.features().contains("urn:xmpp:carbons:2")) {
            cm->setCarbonsEnabled(true);
        }
    }
}

void Core::Account::cancelHistoryRequests()
{
    for (const std::pair<const QString, Conference*>& pair : conferences) {
        pair.second->clearArchiveRequests();
    }
    for (const std::pair<const QString, Contact*>& pair : contacts) {
        pair.second->clearArchiveRequests();
    }
    achiveQueries.clear();
}

