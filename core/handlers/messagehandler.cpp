/*
 * Squawk messenger. 
 * Copyright (C) 2019  Yury Gubich <blue@macaw.me>
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

#include "messagehandler.h"
#include "core/account.h"

Core::MessageHandler::MessageHandler(Core::Account* account):
    QObject(),
    acc(account),
    pendingStateMessages(),
    pendingMessages(),
    uploadingSlotsQueue()
{
}

void Core::MessageHandler::onMessageReceived(const QXmppMessage& msg)
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
        case QXmppMessage::Error: {
            QString id = msg.id();
            std::map<QString, QString>::const_iterator itr = pendingStateMessages.find(id);
            if (itr != pendingStateMessages.end()) {
                QString jid = itr->second;
                RosterItem* cnt = acc->rh->getRosterItem(jid);
                QMap<QString, QVariant> cData = {
                    {"state", static_cast<uint>(Shared::Message::State::error)},
                    {"errorText", msg.error().text()}
                };
                if (cnt != 0) {
                    cnt->changeMessage(id, cData);
                }
                ;
                emit acc->changeMessage(jid, id, cData);
                pendingStateMessages.erase(itr);
                handled = true;
            } else {
                qDebug() << "received a message with type \"Error\", not sure what to do with it now, skipping";
            }
        }
        break;
        case QXmppMessage::Headline:
            qDebug() << "received a message with type \"Headline\", not sure what to do with it now, skipping";
            break;
    }
    if (!handled) {
        logMessage(msg);
    }
}

bool Core::MessageHandler::handleChatMessage(const QXmppMessage& msg, bool outgoing, bool forwarded, bool guessing)
{
    const QString& body(msg.body());
    if (body.size() != 0) {
        Shared::Message sMsg(Shared::Message::chat);
        initializeMessage(sMsg, msg, outgoing, forwarded, guessing);
        QString jid = sMsg.getPenPalJid();
        Contact* cnt = acc->rh->getContact(jid);
        if (cnt == 0) {
            cnt = acc->rh->addOutOfRosterContact(jid);
        }
        if (outgoing) {
            if (forwarded) {
                sMsg.setState(Shared::Message::State::sent);
            }
        } else {
            sMsg.setState(Shared::Message::State::delivered);
        }
        QString oId = msg.replaceId();
        if (oId.size() > 0) {
            QMap<QString, QVariant> cData = {
                {"body", sMsg.getBody()},
                {"stamp", sMsg.getTime()}
            };
            cnt->correctMessageInArchive(oId, sMsg);
            emit acc->changeMessage(jid, oId, cData);
        } else {
            cnt->appendMessageToArchive(sMsg);
            emit acc->message(sMsg);
        }
        
        return true;
    }
    return false;
}

bool Core::MessageHandler::handleGroupMessage(const QXmppMessage& msg, bool outgoing, bool forwarded, bool guessing)
{
    const QString& body(msg.body());
    if (body.size() != 0) {
        QString id = msg.id();
        
        Shared::Message sMsg(Shared::Message::groupChat);
        initializeMessage(sMsg, msg, outgoing, forwarded, guessing);
        QString jid = sMsg.getPenPalJid();
        Conference* cnt = acc->rh->getConference(jid);
        if (cnt == 0) {
            return false;
        }
        
        std::map<QString, QString>::const_iterator pItr = pendingStateMessages.find(id);
        if (pItr != pendingStateMessages.end()) {
            QMap<QString, QVariant> cData = {{"state", static_cast<uint>(Shared::Message::State::delivered)}};
            cnt->changeMessage(id, cData);
            pendingStateMessages.erase(pItr);
            emit acc->changeMessage(jid, id, cData);
        } else {
            QString oId = msg.replaceId();
            if (oId.size() > 0) {
                QMap<QString, QVariant> cData = {
                    {"body", sMsg.getBody()},
                    {"stamp", sMsg.getTime()}
                };
                cnt->correctMessageInArchive(oId, sMsg);
                emit acc->changeMessage(jid, oId, cData);
            } else {
                cnt->appendMessageToArchive(sMsg);
                QDateTime minAgo = QDateTime::currentDateTimeUtc().addSecs(-60);
                if (sMsg.getTime() > minAgo) {     //otherwise it's considered a delayed delivery, most probably MUC history receipt
                    emit acc->message(sMsg);
                } else {
                    //qDebug() << "Delayed delivery: ";
                }
            }
        }
        
        return true;
    } 
    return false;
}


void Core::MessageHandler::initializeMessage(Shared::Message& target, const QXmppMessage& source, bool outgoing, bool forwarded, bool guessing) const
{
    const QDateTime& time(source.stamp());
    QString id;
#if (QXMPP_VERSION) >= QT_VERSION_CHECK(1, 3, 0)
    id = source.originId();
    if (id.size() == 0) {
        id = source.id();
    }
    target.setStanzaId(source.stanzaId());
#else
    id = source.id();
#endif
    target.setId(id);
    QString messageId = target.getId();
    if (messageId.size() == 0) {
        target.generateRandomId();          //TODO out of desperation, I need at least a random ID
        messageId = target.getId();
    }
    target.setFrom(source.from());
    target.setTo(source.to());
    target.setBody(source.body());
    target.setForwarded(forwarded);
    
    if (guessing) {
        if (target.getFromJid() == acc->getLogin() + "@" + acc->getServer()) {
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
    
    QString oob = source.outOfBandUrl();
    if (oob.size() > 0) {
        target.setAttachPath(acc->network->addMessageAndCheckForPath(oob, acc->getName(), target.getPenPalJid(), messageId));
    }
    target.setOutOfBandUrl(oob);
}

void Core::MessageHandler::logMessage(const QXmppMessage& msg, const QString& reason)
{
    qDebug() << reason;
    qDebug() << "- from: " << msg.from();
    qDebug() << "- to: " << msg.to();
    qDebug() << "- body: " << msg.body();
    qDebug() << "- type: " << msg.type();
    qDebug() << "- state: " << msg.state();
    qDebug() << "- stamp: " << msg.stamp();
    qDebug() << "- id: " << msg.id();
#if (QXMPP_VERSION) >= QT_VERSION_CHECK(1, 3, 0)
    qDebug() << "- stanzaId: " << msg.stanzaId();
#endif
    qDebug() << "- outOfBandUrl: " << msg.outOfBandUrl();
    qDebug() << "==============================";
}

void Core::MessageHandler::onCarbonMessageReceived(const QXmppMessage& msg)
{
    handleChatMessage(msg, false, true);
}

void Core::MessageHandler::onCarbonMessageSent(const QXmppMessage& msg)
{
    handleChatMessage(msg, true, true);
}

void Core::MessageHandler::onReceiptReceived(const QString& jid, const QString& id)
{
    std::map<QString, QString>::const_iterator itr = pendingStateMessages.find(id);
    if (itr != pendingStateMessages.end()) {
        QMap<QString, QVariant> cData = {{"state", static_cast<uint>(Shared::Message::State::delivered)}};
        RosterItem* ri = acc->rh->getRosterItem(itr->second);
        if (ri != 0) {
            ri->changeMessage(id, cData);
        }
        pendingStateMessages.erase(itr);
        emit acc->changeMessage(itr->second, id, cData);
    }
}

void Core::MessageHandler::sendMessage(const Shared::Message& data)
{
    if (data.getOutOfBandUrl().size() == 0 && data.getAttachPath().size() > 0) {
        prepareUpload(data);
    } else {
        performSending(data);
    }
}

void Core::MessageHandler::performSending(Shared::Message data)
{
    QString jid = data.getPenPalJid();
    QString id = data.getId();
    QString oob = data.getOutOfBandUrl();
    RosterItem* ri = acc->rh->getRosterItem(jid);
    QMap<QString, QVariant> changes;
    if (acc->state == Shared::ConnectionState::connected) {
        QXmppMessage msg(acc->getFullJid(), data.getTo(), data.getBody(), data.getThread());
        
#if (QXMPP_VERSION) >= QT_VERSION_CHECK(1, 3, 0)
        msg.setOriginId(id);
#endif
        msg.setId(id);
        msg.setType(static_cast<QXmppMessage::Type>(data.getType()));       //it is safe here, my type is compatible
        msg.setOutOfBandUrl(oob);
        msg.setReceiptRequested(true);
        
        bool sent = acc->client.sendPacket(msg);
        
        if (sent) {
            data.setState(Shared::Message::State::sent);
        } else {
            data.setState(Shared::Message::State::error);
            data.setErrorText("Couldn't send message via QXMPP library check out logs");
        }
        
        if (ri != 0) {
            ri->appendMessageToArchive(data);
            if (sent) {
                pendingStateMessages.insert(std::make_pair(id, jid));
            }
        }
        
    } else {
        data.setState(Shared::Message::State::error);
        data.setErrorText("You are is offline or reconnecting");
    }
    
    Shared::Message::State mstate = data.getState();
    changes.insert("state", static_cast<uint>(mstate));
    if (mstate == Shared::Message::State::error) {
        changes.insert("errorText", data.getErrorText());
    }
    if (oob.size() > 0) {
        changes.insert("outOfBandUrl", oob);
    }
    
    emit acc->changeMessage(jid, id, changes);
}

void Core::MessageHandler::prepareUpload(const Shared::Message& data)
{
    if (acc->state == Shared::ConnectionState::connected) {
        QString path = data.getAttachPath();
        QString url = acc->network->getFileRemoteUrl(path);
        if (url.size() != 0) {
            sendMessageWithLocalUploadedFile(data, url);
        } else {
            if (acc->network->checkAndAddToUploading(acc->getName(), data.getPenPalJid(), data.getId(), path)) {
                pendingMessages.emplace(data.getId(), data);
            } else {
                if (acc->um->serviceFound()) {
                    QFileInfo file(path);
                    if (file.exists() && file.isReadable()) {
                        uploadingSlotsQueue.emplace_back(path, data);
                        if (uploadingSlotsQueue.size() == 1) {
                            acc->um->requestUploadSlot(file);
                        }
                    } else {
                        handleUploadError(data.getPenPalJid(), data.getId(), "Uploading file no longer exists or your system user has no permission to read it");
                        qDebug() << "Requested upload slot in account" << acc->name << "for file" << path << "but the file doesn't exist or is not readable";
                    }
                } else {
                    handleUploadError(data.getPenPalJid(), data.getId(), "Your server doesn't support file upload service, or it's prohibited for your account");
                    qDebug() << "Requested upload slot in account" << acc->name << "for file" << path << "but upload manager didn't discover any upload services";
                }
            }
        }
    } else {
        handleUploadError(data.getPenPalJid(), data.getId(), "Account is offline or reconnecting");
        qDebug() << "An attempt to send message with not connected account " << acc->name << ", skipping";
    }
}


void Core::MessageHandler::onUploadSlotReceived(const QXmppHttpUploadSlotIq& slot)
{
    if (uploadingSlotsQueue.size() == 0) {
        qDebug() << "HTTP Upload manager of account" << acc->name << "reports about success requesting upload slot, but none was requested";
    } else {
        const std::pair<QString, Shared::Message>& pair = uploadingSlotsQueue.front();
        const QString& mId = pair.second.getId();
        acc->network->uploadFile({acc->name, pair.second.getPenPalJid(), mId}, pair.first, slot.putUrl(), slot.getUrl(), slot.putHeaders());
        pendingMessages.emplace(mId, pair.second);
        
        uploadingSlotsQueue.pop_front();
        if (uploadingSlotsQueue.size() > 0) {
            acc->um->requestUploadSlot(uploadingSlotsQueue.front().first);
        }
    }
}

void Core::MessageHandler::onUploadSlotRequestFailed(const QXmppHttpUploadRequestIq& request)
{
    QString err(request.error().text());
    if (uploadingSlotsQueue.size() == 0) {
        qDebug() << "HTTP Upload manager of account" << acc->name << "reports about an error requesting upload slot, but none was requested";
        qDebug() << err;
    } else {
        const std::pair<QString, Shared::Message>& pair = uploadingSlotsQueue.front();
        qDebug() << "Error requesting upload slot for file" << pair.first << "in account" << acc->name << ":" << err;
        emit acc->uploadFileError(pair.second.getPenPalJid(), pair.second.getId(), "Error requesting slot to upload file: " + err);
        
        uploadingSlotsQueue.pop_front();
        if (uploadingSlotsQueue.size() > 0) {
            acc->um->requestUploadSlot(uploadingSlotsQueue.front().first);
        }
    }
}

void Core::MessageHandler::onDownloadFileComplete(const std::list<Shared::MessageInfo>& msgs, const QString& path)
{
    QMap<QString, QVariant> cData = {
        {"attachPath", path}
    };
    for (const Shared::MessageInfo& info : msgs) {
        if (info.account == acc->getName()) {
            RosterItem* cnt = acc->rh->getRosterItem(info.jid);
            if (cnt != 0) {
                if (cnt->changeMessage(info.messageId, cData)) {
                    emit acc->changeMessage(info.jid, info.messageId, cData);
                }
            }
        }
    }
}

void Core::MessageHandler::onLoadFileError(const std::list<Shared::MessageInfo>& msgs, const QString& text, bool up)
{
    if (up) {
        for (const Shared::MessageInfo& info : msgs) {
            if (info.account == acc->getName()) {
                handleUploadError(info.jid, info.messageId, text);
            }
        }
    }
}

void Core::MessageHandler::handleUploadError(const QString& jid, const QString& messageId, const QString& errorText)
{
    std::map<QString, Shared::Message>::const_iterator itr = pendingMessages.find(messageId);
    if (itr != pendingMessages.end()) {
        pendingMessages.erase(itr);
        //TODO move the storage of pending messages to the database and change them there
    }
}

void Core::MessageHandler::onUploadFileComplete(const std::list<Shared::MessageInfo>& msgs, const QString& path)
{
    for (const Shared::MessageInfo& info : msgs) {
        if (info.account == acc->getName()) {
            std::map<QString, Shared::Message>::const_iterator itr = pendingMessages.find(info.messageId);
            if (itr != pendingMessages.end()) {
                sendMessageWithLocalUploadedFile(itr->second, path);
                pendingMessages.erase(itr);
            }
        }
    }
}

void Core::MessageHandler::sendMessageWithLocalUploadedFile(Shared::Message msg, const QString& url)
{
    msg.setOutOfBandUrl(url);
    if (msg.getBody().size() == 0) {
        msg.setBody(url);
    }
    performSending(msg);
    //TODO removal/progress update
}

void Core::MessageHandler::requestChangeMessage(const QString& jid, const QString& messageId, const QMap<QString, QVariant>& data)
{
    RosterItem* cnt = acc->rh->getRosterItem(jid);
    if (cnt != 0) {
        QMap<QString, QVariant>::const_iterator itr = data.find("attachPath");
        if (data.size() == 1 && itr != data.end()) {
            cnt->changeMessage(messageId, data);
            emit acc->changeMessage(jid, messageId, data);
        } else {
            qDebug() << "A request to change message" << messageId << "of conversation" << jid << "with following data" << data;
            qDebug() << "nothing but the changing of the local path is supported yet in this method, skipping";
        }
    }
}
