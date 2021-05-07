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

#ifndef CORE_MESSAGEHANDLER_H
#define CORE_MESSAGEHANDLER_H

#include <QObject>

#include <deque>
#include <map>

#include <QXmppMessage.h>
#include <QXmppHttpUploadIq.h>

#include <shared/message.h>
#include <shared/messageinfo.h>

namespace Core {

/**
 * @todo write docs
 */

class Account;

class MessageHandler : public QObject
{
    Q_OBJECT
public:
    MessageHandler(Account* account);
    
public:
    void sendMessage(const Shared::Message& data);
    void initializeMessage(Shared::Message& target, const QXmppMessage& source, bool outgoing = false, bool forwarded = false, bool guessing = false) const;
    
public slots:
    void onMessageReceived(const QXmppMessage& message);
    void onCarbonMessageReceived(const QXmppMessage& message);
    void onCarbonMessageSent(const QXmppMessage& message);
    void onReceiptReceived(const QString& jid, const QString& id);    
    void onUploadSlotReceived(const QXmppHttpUploadSlotIq& slot);
    void onUploadSlotRequestFailed(const QXmppHttpUploadRequestIq& request);
    void onDownloadFileComplete(const std::list<Shared::MessageInfo>& msgs, const QString& path);
    void onUploadFileComplete(const std::list<Shared::MessageInfo>& msgs, const QString& path);
    void onLoadFileError(const std::list<Shared::MessageInfo>& msgs, const QString& path, bool up);
    void requestChangeMessage(const QString& jid, const QString& messageId, const QMap<QString, QVariant>& data);
    
private:
    bool handleChatMessage(const QXmppMessage& msg, bool outgoing = false, bool forwarded = false, bool guessing = false);
    bool handleGroupMessage(const QXmppMessage& msg, bool outgoing = false, bool forwarded = false, bool guessing = false);
    void logMessage(const QXmppMessage& msg, const QString& reason = "Message wasn't handled: ");
    void sendMessageWithLocalUploadedFile(Shared::Message msg, const QString& url, bool newMessage = true);
    void performSending(Shared::Message data, bool newMessage = true);
    void prepareUpload(const Shared::Message& data);
    void handleUploadError(const QString& jid, const QString& messageId, const QString& errorText);
    
private:
    Account* acc;
    std::map<QString, QString> pendingStateMessages;        //key is message id, value is JID
    std::deque<std::pair<QString, QString>> uploadingSlotsQueue;
};

}

#endif // CORE_MESSAGEHANDLER_H
