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
    cm(new QXmppCarbonManager()),
    am(new QXmppMamManager()),
    mm(new QXmppMucManager()),
    bm(new QXmppBookmarkManager()),
    rm(client.findExtension<QXmppRosterManager>()),
    vm(client.findExtension<QXmppVCardManager>()),
    um(new QXmppUploadRequestManager()),
    dm(client.findExtension<QXmppDiscoveryManager>()),
    rcpm(new QXmppMessageReceiptManager()),
    reconnectScheduled(false),
    reconnectTimer(new QTimer),
    avatarHash(),
    avatarType(),
    ownVCardRequestInProgress(false),
    network(p_net),
    passwordType(Shared::AccountPassword::plain),
    mh(new MessageHandler(this)),
    rh(new RosterHandler(this))
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
    
    client.addExtension(cm);
    
    QObject::connect(cm, &QXmppCarbonManager::messageReceived, mh, &MessageHandler::onCarbonMessageReceived);
    QObject::connect(cm, &QXmppCarbonManager::messageSent, mh, &MessageHandler::onCarbonMessageSent);
    
    client.addExtension(am);
    
    QObject::connect(am, &QXmppMamManager::logMessage, this, &Account::onMamLog);
    QObject::connect(am, &QXmppMamManager::archivedMessageReceived, this, &Account::onMamMessageReceived);
    QObject::connect(am, &QXmppMamManager::resultsRecieved, this, &Account::onMamResultsReceived);
    
    client.addExtension(mm);
    client.addExtension(bm);
    
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
    
    delete mh;
    delete rh;
    
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
        rh->clearConferences();
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

QString Core::Account::getName() const {
    return name;}

QString Core::Account::getLogin() const {
    return config.user();}

QString Core::Account::getPassword() const {
    return config.password();}

QString Core::Account::getServer() const {
    return config.domain();}

Shared::AccountPassword Core::Account::getPasswordType() const {
    return passwordType;}

void Core::Account::setPasswordType(Shared::AccountPassword pt) {
    passwordType = pt; }

void Core::Account::setLogin(const QString& p_login) {
    config.setUser(p_login);}

void Core::Account::setName(const QString& p_name) {
    name = p_name;}

void Core::Account::setPassword(const QString& p_password) {
    config.setPassword(p_password);}

void Core::Account::setServer(const QString& p_server) {
    config.setDomain(p_server);}

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
        RosterItem* item = rh->getRosterItem(jid);
        if (item != 0) {
            item->handlePresence(p_presence);
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

QString Core::Account::getResource() const {
    return config.resource();}

void Core::Account::setResource(const QString& p_resource) {
    config.setResource(p_resource);}

QString Core::Account::getFullJid() const {
    return getLogin() + "@" + getServer() + "/" + getResource();}

void Core::Account::sendMessage(const Shared::Message& data) {
    mh->sendMessage(data);}

void Core::Account::sendMessage(const Shared::Message& data, const QString& path) {
    mh->sendMessage(data, path);}

void Core::Account::onMamMessageReceived(const QString& queryId, const QXmppMessage& msg)
{
    if (msg.id().size() > 0 && (msg.body().size() > 0 || msg.outOfBandUrl().size() > 0)) {
        std::map<QString, QString>::const_iterator itr = achiveQueries.find(queryId);
        if (itr != achiveQueries.end()) {
            QString jid = itr->second;
            RosterItem* item = rh->getRosterItem(jid);
            
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
}

void Core::Account::requestArchive(const QString& jid, int count, const QString& before)
{
    qDebug() << "An archive request for " << jid << ", before " << before;
    RosterItem* contact = rh->getRosterItem(jid);
    
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
        
        RosterItem* ri = rh->getRosterItem(jid);
        
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

void Core::Account::removeContactRequest(const QString& jid) {
    rh->removeContactRequest(jid);}

void Core::Account::addContactRequest(const QString& jid, const QString& name, const QSet<QString>& groups) {
    rh->addContactRequest(jid, name, groups);}

void Core::Account::setRoomAutoJoin(const QString& jid, bool joined)
{
    Conference* conf = rh->getConference(jid);
    if (conf == 0) {
        qDebug() << "An attempt to set auto join to the non existing room" << jid << "of the account" << getName() << ", skipping";
        return;
    }
    
    conf->setAutoJoin(joined);
}

void Core::Account::setRoomJoined(const QString& jid, bool joined)
{
    Conference* conf = rh->getConference(jid);
    if (conf == 0) {
        qDebug() << "An attempt to set joined to the non existing room" << jid << "of the account" << getName() << ", skipping";
        return;
    }
    
    conf->setJoined(joined);
}

void Core::Account::removeRoomRequest(const QString& jid){
    rh->removeRoomRequest(jid);}

void Core::Account::addRoomRequest(const QString& jid, const QString& nick, const QString& password, bool autoJoin) {
    rh->addRoomRequest(jid, nick, password, autoJoin);}

void Core::Account::addContactToGroupRequest(const QString& jid, const QString& groupName) {
    rh->addContactToGroupRequest(jid, groupName);}

void Core::Account::removeContactFromGroupRequest(const QString& jid, const QString& groupName) {
    rh->removeContactFromGroupRequest(jid, groupName);}

void Core::Account::renameContactRequest(const QString& jid, const QString& newName)
{
    Contact* cnt = rh->getContact(jid);
    if (cnt == 0) {
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
    RosterItem* item = rh->getRosterItem(jid);
    
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
    rh->cancelHistoryRequests();
    achiveQueries.clear();
}

