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

Account::Account(const QString& p_login, const QString& p_server, const QString& p_password, const QString& p_name, NetworkAccess* p_net, QObject* parent):
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
    rm(client.findExtension<QXmppRosterManager>()),
    vm(client.findExtension<QXmppVCardManager>()),
    um(new QXmppUploadRequestManager()),
    dm(client.findExtension<QXmppDiscoveryManager>()),
    contacts(),
    conferences(),
    maxReconnectTimes(0),
    reconnectTimes(0),
    queuedContacts(),
    outOfRosterContacts(),
    pendingMessages(),
    uploadingSlotsQueue(),
    avatarHash(),
    avatarType(),
    ownVCardRequestInProgress(false),
    network(p_net)
{
    config.setUser(p_login);
    config.setDomain(p_server);
    config.setPassword(p_password);
    config.setAutoAcceptSubscriptions(true);
    
    QObject::connect(&client, &QXmppClient::connected, this, &Account::onClientConnected);
    QObject::connect(&client, &QXmppClient::disconnected, this, &Account::onClientDisconnected);
    QObject::connect(&client, &QXmppClient::presenceReceived, this, &Account::onPresenceReceived);
    QObject::connect(&client, &QXmppClient::messageReceived, this, &Account::onMessageReceived);
    QObject::connect(&client, &QXmppClient::error, this, &Account::onClientError);
    
    QObject::connect(rm, &QXmppRosterManager::rosterReceived, this, &Account::onRosterReceived);
    QObject::connect(rm, &QXmppRosterManager::itemAdded, this, &Account::onRosterItemAdded);
    QObject::connect(rm, &QXmppRosterManager::itemRemoved, this, &Account::onRosterItemRemoved);
    QObject::connect(rm, &QXmppRosterManager::itemChanged, this, &Account::onRosterItemChanged);
    //QObject::connect(&rm, &QXmppRosterManager::presenceChanged, this, &Account::onRosterPresenceChanged);
    
    client.addExtension(cm);
    
    QObject::connect(cm, &QXmppCarbonManager::messageReceived, this, &Account::onCarbonMessageReceived);
    QObject::connect(cm, &QXmppCarbonManager::messageSent, this, &Account::onCarbonMessageSent);
    
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
    QObject::connect(um, &QXmppUploadRequestManager::slotReceived, this, &Account::onUploadSlotReceived);
    QObject::connect(um, &QXmppUploadRequestManager::requestFailed, this, &Account::onUploadSlotRequestFailed);
    
    QObject::connect(dm, &QXmppDiscoveryManager::itemsReceived, this, &Account::onDiscoveryItemsReceived);
    QObject::connect(dm, &QXmppDiscoveryManager::infoReceived, this, &Account::onDiscoveryInfoReceived);
    
    QObject::connect(network, &NetworkAccess::uploadFileComplete, this, &Account::onFileUploaded);
    QObject::connect(network, &NetworkAccess::uploadFileError, this, &Account::onFileUploadError);
    
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
}

Account::~Account()
{
    QObject::disconnect(network, &NetworkAccess::uploadFileComplete, this, &Account::onFileUploaded);
    QObject::disconnect(network, &NetworkAccess::uploadFileError, this, &Account::onFileUploadError);
    
    for (std::map<QString, Contact*>::const_iterator itr = contacts.begin(), end = contacts.end(); itr != end; ++itr) {
        delete itr->second;
    }
    
    for (std::map<QString, Conference*>::const_iterator itr = conferences.begin(), end = conferences.end(); itr != end; ++itr) {
        delete itr->second;
    }
    
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
        clearConferences();
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
        dm->requestItems(getServer());
        emit connectionStateChanged(state);
    } else {
        qDebug() << "Something weird had happened - xmpp client reported about successful connection but account wasn't in" << state << "state";
    }
}

void Core::Account::onClientDisconnected()
{
    clearConferences();
    if (state != Shared::disconnected) {
        if (reconnectTimes > 0) {
            qDebug() << "Account" << name << "is reconnecting for" << reconnectTimes << "more times";
            --reconnectTimes;
            state = Shared::connecting;
            client.connectToServer(config, presence);
            emit connectionStateChanged(state);
        } else {
            qDebug() << "Account" << name << "has been disconnected";
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
    vm->requestClientVCard();         //TODO need to make sure server actually supports vCards
    ownVCardRequestInProgress = true;
    
    QStringList bj = rm->getRosterBareJids();
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
        rm->subscribe(bareJid, itr->second);
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
            {"state", state}
        });
        
        if (contact->hasAvatar()) {
            if (!contact->isAvatarAutoGenerated()) {
                cData.insert("avatarState", static_cast<uint>(Shared::Avatar::valid));
            } else {
                cData.insert("avatarState", static_cast<uint>(Shared::Avatar::autocreated));
            }
            cData.insert("avatarPath", contact->avatarPath());
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
            emit availabilityChanged(p_presence.availableStatusType());
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
            RosterItem* item = 0;
            std::map<QString, Contact*>::const_iterator itr = contacts.find(jid);
            if (itr != contacts.end()) {
                item = itr->second;
            } else {
                std::map<QString, Conference*>::const_iterator citr = conferences.find(jid);
                if (citr != conferences.end()) {
                    item = citr->second;
                }
            }
            
            if (item != 0) {
                switch (p_presence.vCardUpdateType()) {
                    case QXmppPresence::VCardUpdateNone:            //this presence has nothing to do with photo
                        break;
                    case QXmppPresence::VCardUpdateNotReady:        //let's say the photo didn't change here
                        break;
                    case QXmppPresence::VCardUpdateNoPhoto:         //there is no photo, need to drop if any
                        if (!item->hasAvatar() || (item->hasAvatar() && !item->isAvatarAutoGenerated())) {
                            item->setAutoGeneratedAvatar();
                        }
                        break;
                    case QXmppPresence::VCardUpdateValidPhoto:      //there is a photo, need to load
                        if (item->hasAvatar()) {
                            if (item->isAvatarAutoGenerated()) {
                                requestVCard(jid);
                            } else {
                                if (item->avatarHash() != p_presence.photoHash()) {
                                    requestVCard(jid);
                                }
                            }
                        } else {
                            requestVCard(jid);
                        }
                        break;
                }
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
                lastInteraction = QDateTime::currentDateTime();
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
            handled = handleGroupMessage(msg);
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
        qDebug() << "- outOfBandUrl: " << msg.outOfBandUrl();
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
        msg.setOutOfBandUrl(data.getOutOfBandUrl());
        
        RosterItem* ri = 0;
        std::map<QString, Contact*>::const_iterator itr = contacts.find(data.getPenPalJid());
        if (itr != contacts.end()) {
            ri = itr->second;
        } else {
            std::map<QString, Conference*>::const_iterator ritr = conferences.find(data.getPenPalJid());
            if (ritr != conferences.end()) {
                ri = ritr->second;
            }
        }
        
        if (ri != 0) {
            if (!ri->isMuc()) {
                ri->appendMessageToArchive(data);
            }
        }
        
        client.sendPacket(msg);
        
    } else {
        qDebug() << "An attempt to send message with not connected account " << name << ", skipping";
    }
}

void Core::Account::sendMessage(const Shared::Message& data, const QString& path)
{
    if (state == Shared::connected) {
        QString url = network->getFileRemoteUrl(path);
        if (url.size() != 0) {
            sendMessageWithLocalUploadedFile(data, url);
        } else {
            if (network->isUploading(path, data.getId())) {
                pendingMessages.emplace(data.getId(), data);
            } else {
                if (um->serviceFound()) {
                    QFileInfo file(path);
                    if (file.exists() && file.isReadable()) {
                        uploadingSlotsQueue.emplace_back(path, data);
                        if (uploadingSlotsQueue.size() == 1) {
                            um->requestUploadSlot(file);
                        }
                    } else {
                        emit onFileUploadError(data.getId(), "Uploading file dissapeared or your system user has no permission to read it");
                        qDebug() << "Requested upload slot in account" << name << "for file" << path << "but the file doesn't exist or is not readable";
                    }
                } else {
                    emit onFileUploadError(data.getId(), "Your server doesn't support file upload service, or it's prohibited for your account");
                    qDebug() << "Requested upload slot in account" << name << "for file" << path << "but upload manager didn't discover any upload services";
                }
            }
        }
    } else {
        emit onFileUploadError(data.getId(), "Account is offline or reconnecting");
        qDebug() << "An attempt to send message with not connected account " << name << ", skipping";
    }
}

void Core::Account::sendMessageWithLocalUploadedFile(Shared::Message msg, const QString& url)
{
    msg.setOutOfBandUrl(url);
    if (msg.getBody().size() == 0) {
        msg.setBody(url);
    }
    sendMessage(msg);
    //TODO removal/progress update
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

bool Core::Account::handleGroupMessage(const QXmppMessage& msg, bool outgoing, bool forwarded, bool guessing)
{
    const QString& body(msg.body());
    if (body.size() != 0) {
        const QString& id(msg.id());
        Shared::Message sMsg(Shared::Message::groupChat);
        initializeMessage(sMsg, msg, outgoing, forwarded, guessing);
        QString jid = sMsg.getPenPalJid();
        std::map<QString, Conference*>::const_iterator itr = conferences.find(jid);
        Conference* cnt;
        if (itr != conferences.end()) {
            cnt = itr->second;
        } else {
            return false;
        }
        cnt->appendMessageToArchive(sMsg);
        
        QDateTime fiveMinsAgo = QDateTime::currentDateTime().addSecs(-300);
        if (sMsg.getTime() > fiveMinsAgo) {     //otherwise it's considered a delayed delivery, most probably MUC history receipt
            emit message(sMsg);
        }
        
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
    target.setOutOfBandUrl(source.outOfBandUrl());
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
    if (msg.id().size() > 0 && (msg.body().size() > 0 || msg.outOfBandUrl().size() > 0)) {
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
    bool gr = false;
    if (itr != contacts.end()) {
        contact = itr->second;
    } else {
        std::map<QString, Conference*>::const_iterator citr = conferences.find(jid);
        if (citr != conferences.end()) {
            contact = citr->second;
            gr = true;
        }
    }
    
    if (contact == 0) {
        qDebug() << "An attempt to request archive for" << jid << "in account" << name << ", but the contact with such id wasn't found, skipping";
        emit responseArchive(contact->jid, std::list<Shared::Message>());
        return;
    }
    
    if (contact->getArchiveState() == RosterItem::empty && before.size() == 0) {
        QXmppMessage msg(getFullJid(), jid, "", "");
        QString last = Shared::generateUUID();
        msg.setId(last);
        if (gr) {
            msg.setType(QXmppMessage::GroupChat);
        } else {
            msg.setType(QXmppMessage::Chat);
        }
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
    QString to = "";
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
    
    QString q = am->retrieveArchivedMessages(to, "", contact->jid, start, QDateTime(), query);
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
        case QXmppClient::NoError:
            break;                      //not exactly sure what to do here
    }
    
    qDebug() << errorType << errorText;
    emit error(errorText);
}


void Core::Account::subscribeToContact(const QString& jid, const QString& reason)
{
    if (state == Shared::connected) {
        rm->subscribe(jid, reason);
    } else {
        qDebug() << "An attempt to subscribe account " << name << " to contact " << jid << " but the account is not in the connected state, skipping";
    }
}

void Core::Account::unsubscribeFromContact(const QString& jid, const QString& reason)
{
    if (state == Shared::connected) {
        rm->unsubscribe(jid, reason);
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
            rm->removeItem(jid);
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
        {"name", conf->getName()}
    };
    
    if (conf->hasAvatar()) {
        if (!conf->isAvatarAutoGenerated()) {
            cData.insert("avatarState", static_cast<uint>(Shared::Avatar::valid));
        } else {
            cData.insert("avatarState", static_cast<uint>(Shared::Avatar::autocreated));
        }
        cData.insert("avatarPath", conf->avatarPath());
    } else {
        cData.insert("avatarState", static_cast<uint>(Shared::Avatar::empty));
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
    QString jid = card.from();
    pendingVCardRequests.erase(jid);
    RosterItem* item = 0;
    
    std::map<QString, Contact*>::const_iterator contItr = contacts.find(jid);
    if (contItr == contacts.end()) {
        std::map<QString, Conference*>::const_iterator confItr = conferences.find(jid);
        if (confItr == conferences.end()) {
            if (jid == getLogin() + "@" + getServer()) {
                onOwnVCardReceived(card);
            } else {
                qDebug() << "received vCard" << jid << "doesn't belong to any of known contacts or conferences, skipping";
            }
            return;
        } else {
            item = confItr->second;
        }
    } else {
        item = contItr->second;
    }
    
    QByteArray ava = card.photo();
    
    if (ava.size() > 0) {
        item->setAvatar(ava);
    } else {
        if (!item->hasAvatar() || !item->isAvatarAutoGenerated()) {
            item->setAutoGeneratedAvatar();
        }
    }
    
    Shared::VCard vCard;
    initializeVCard(vCard, card);
    
    if (item->hasAvatar()) {
        if (!item->isAvatarAutoGenerated()) {
            vCard.setAvatarType(Shared::Avatar::valid);
        } else {
            vCard.setAvatarType(Shared::Avatar::autocreated);
        }
        vCard.setAvatarPath(item->avatarPath());
    } else {
        vCard.setAvatarType(Shared::Avatar::empty);
    }
    
    QMap<QString, QVariant> cd = {
        {"avatarState", static_cast<quint8>(vCard.getAvatarType())},
        {"avatarPath", vCard.getAvatarPath()}
    };
    emit changeContact(jid, cd);
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

void Core::Account::onUploadSlotReceived(const QXmppHttpUploadSlotIq& slot)
{
    if (uploadingSlotsQueue.size() == 0) {
        qDebug() << "HTTP Upload manager of account" << name << "reports about success requesting upload slot, but none was requested";
    } else {
        const std::pair<QString, Shared::Message>& pair = uploadingSlotsQueue.front();
        const QString& mId = pair.second.getId();
        network->uploadFile(mId, pair.first, slot.putUrl(), slot.getUrl(), slot.putHeaders());
        pendingMessages.emplace(mId, pair.second);
        uploadingSlotsQueue.pop_front();
        
        if (uploadingSlotsQueue.size() > 0) {
            um->requestUploadSlot(uploadingSlotsQueue.front().first);
        }
    }
}

void Core::Account::onUploadSlotRequestFailed(const QXmppHttpUploadRequestIq& request)
{
    if (uploadingSlotsQueue.size() == 0) {
        qDebug() << "HTTP Upload manager of account" << name << "reports about an error requesting upload slot, but none was requested";
        qDebug() << request.error().text();
    } else {
        const std::pair<QString, Shared::Message>& pair = uploadingSlotsQueue.front();
        qDebug() << "Error requesting upload slot for file" << pair.first << "in account" << name << ":" << request.error().text();
        emit uploadFileError(pair.second.getId(), "Error requesting slot to upload file: " + request.error().text());
        
        if (uploadingSlotsQueue.size() > 0) {
            um->requestUploadSlot(uploadingSlotsQueue.front().first);
        }
        uploadingSlotsQueue.pop_front();
    }
}

void Core::Account::onFileUploaded(const QString& messageId, const QString& url)
{
    std::map<QString, Shared::Message>::const_iterator itr = pendingMessages.find(messageId);
    if (itr != pendingMessages.end()) {
        sendMessageWithLocalUploadedFile(itr->second, url);
        pendingMessages.erase(itr);
    }
}

void Core::Account::onFileUploadError(const QString& messageId, const QString& errMsg)
{
    std::map<QString, Shared::Message>::const_iterator itr = pendingMessages.find(messageId);
    if (itr != pendingMessages.end()) {
        pendingMessages.erase(itr);
    }
}

void Core::Account::onDiscoveryItemsReceived(const QXmppDiscoveryIq& items)
{
    for (QXmppDiscoveryIq::Item item : items.items()) {
        dm->requestInfo(item.jid());
    }
}

void Core::Account::onDiscoveryInfoReceived(const QXmppDiscoveryIq& info)
{

}
