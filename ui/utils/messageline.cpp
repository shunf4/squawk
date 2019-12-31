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

#include "messageline.h"
#include <QDebug>
#include <cmath>

MessageLine::MessageLine(bool p_room, QWidget* parent):
    QWidget(parent),
    messageIndex(),
    messageOrder(),
    myMessages(),
    palMessages(),
    uploadPaths(),
    palAvatars(),
    layout(new QVBoxLayout(this)),
    myName(),
    myAvatarPath(),
    palNames(),
    uploading(),
    downloading(),
    room(p_room),
    busyShown(false),
    progress()
{
    setBackgroundRole(QPalette::Base);
    layout->addStretch();
}

MessageLine::~MessageLine()
{
    for (Index::const_iterator itr = messageIndex.begin(), end = messageIndex.end(); itr != end; ++itr) {
        delete itr->second;
    }
}

MessageLine::Position MessageLine::message(const Shared::Message& msg, bool forceOutgoing)
{
    QString id = msg.getId();
    Index::iterator itr = messageIndex.find(id);
    if (itr != messageIndex.end()) {
        qDebug() << "received more then one message with the same id, skipping yet the new one";
        return invalid;
    }
    
    QString sender;
    QString aPath;
    bool outgoing;
    
    if (forceOutgoing) {
        sender = myName;
        aPath = myAvatarPath;
        outgoing = true;
    } else {
        if (room) {
            if (msg.getFromResource() == myName) {
                sender = myName;
                aPath = myAvatarPath;
                outgoing = true;
            } else {
                sender = msg.getFromResource();
                std::map<QString, QString>::iterator aItr = palAvatars.find(sender);
                if (aItr != palAvatars.end()) {
                    aPath = aItr->second;
                }
                outgoing = false;
            }
        } else {
            if (msg.getOutgoing()) {
                sender = myName;
                aPath = myAvatarPath;
                outgoing = true;
            } else {
                QString jid = msg.getFromJid();
                std::map<QString, QString>::iterator itr = palNames.find(jid);
                if (itr != palNames.end()) {
                    sender = itr->second;
                } else {
                    sender = jid;
                }
                
                std::map<QString, QString>::iterator aItr = palAvatars.find(jid);
                if (aItr != palAvatars.end()) {
                    aPath = aItr->second;
                }
                
                outgoing = false;
            }
        }
    }
    
    Message* message = new Message(msg, outgoing, sender, aPath);
    
    std::pair<Order::const_iterator, bool> result = messageOrder.insert(std::make_pair(msg.getTime(), message));
    if (!result.second) {
        qDebug() << "Error appending a message into a message list - seems like the time of that message exactly matches the time of some other message, can't put them in order, skipping yet";
        delete message;
        return invalid;
    }
    if (outgoing) {
        myMessages.insert(std::make_pair(id, message));
    } else {
        QString senderId;
        if (room) {
            senderId = sender;
        } else {
            QString jid = msg.getFromJid();
        }
        
        std::map<QString, Index>::iterator pItr = palMessages.find(senderId);
        if (pItr == palMessages.end()) {
            pItr = palMessages.insert(std::make_pair(senderId, Index())).first;
        }
        pItr->second.insert(std::make_pair(id, message));
    }
    messageIndex.insert(std::make_pair(id, message));
    unsigned long index = std::distance<Order::const_iterator>(messageOrder.begin(), result.first);   //need to make with binary indexed tree
    Position res = invalid;
    if (index == 0) {
        res = beggining;
    } else if (index == messageIndex.size() - 1) {
        res = end;
    } else {
        res = middle;
    }
    
    if (busyShown) {
        index += 1;
    }
    
        
    if (res == end) {
        layout->addLayout(message);
    } else {
        layout->insertLayout(index, message);
    }
    
    if (msg.hasOutOfBandUrl()) {
        emit requestLocalFile(msg.getId(), msg.getOutOfBandUrl());
        connect(message, &Message::buttonClicked, this, &MessageLine::onDownload);
    }
    
    return res;
}

void MessageLine::onDownload()
{
    Message* msg = static_cast<Message*>(sender());
    QString messageId = msg->getId();
    Index::const_iterator itr = downloading.find(messageId);
    if (itr == downloading.end()) {
        downloading.insert(std::make_pair(messageId, msg));
        msg->setProgress(0);
        msg->showComment(tr("Downloading..."));
        emit downloadFile(messageId, msg->getFileUrl());
    } else {
        qDebug() << "An attempt to initiate download for already downloading file" << msg->getFileUrl() << ", skipping";
    }
}

void MessageLine::setMyName(const QString& name)
{
    myName = name;
    for (Index::const_iterator itr = myMessages.begin(), end = myMessages.end(); itr != end; ++itr) {
        itr->second->setSender(name);
    }
}

void MessageLine::setPalName(const QString& jid, const QString& name)
{
    std::map<QString, QString>::iterator itr = palNames.find(jid);
    if (itr == palNames.end()) {
        palNames.insert(std::make_pair(jid, name));
    } else {
        itr->second = name;
    }
    
    std::map<QString, Index>::iterator pItr = palMessages.find(jid);
    if (pItr != palMessages.end()) {
        for (Index::const_iterator itr = pItr->second.begin(), end = pItr->second.end(); itr != end; ++itr) {
            itr->second->setSender(name);
        }
    }
}

void MessageLine::setPalAvatar(const QString& jid, const QString& path)
{
    std::map<QString, QString>::iterator itr = palAvatars.find(jid);
    if (itr == palAvatars.end()) {
        palAvatars.insert(std::make_pair(jid, path));
    } else {
        itr->second = path;
    }
    
    std::map<QString, Index>::iterator pItr = palMessages.find(jid);
    if (pItr != palMessages.end()) {
        for (Index::const_iterator itr = pItr->second.begin(), end = pItr->second.end(); itr != end; ++itr) {
            itr->second->setAvatarPath(path);
        }
    }
}

void MessageLine::dropPalAvatar(const QString& jid)
{
    std::map<QString, QString>::iterator itr = palAvatars.find(jid);
    if (itr != palAvatars.end()) {
        palAvatars.erase(itr);
        
        std::map<QString, Index>::iterator pItr = palMessages.find(jid);
        if (pItr != palMessages.end()) {
            for (Index::const_iterator itr = pItr->second.begin(), end = pItr->second.end(); itr != end; ++itr) {
                itr->second->setAvatarPath("");
            }
        }
    }
}

void MessageLine::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    emit resize(event->size().height() - event->oldSize().height());
}


QString MessageLine::firstMessageId() const
{
    if (messageOrder.size() == 0) {
        return "";
    } else {
        return messageOrder.begin()->second->getId();
    }
}

void MessageLine::showBusyIndicator()
{
    if (!busyShown)  {
        layout->insertWidget(0, &progress);
        progress.start();
        busyShown = true;
    }
}

void MessageLine::hideBusyIndicator()
{
    if (busyShown) {
        progress.stop();
        layout->removeWidget(&progress);
        busyShown = false;
    }
}

void MessageLine::fileProgress(const QString& messageId, qreal progress)
{
    Index::const_iterator itr = messageIndex.find(messageId);
    if (itr == messageIndex.end()) {
        //TODO may be some logging, that's not normal
    } else {
        itr->second->setProgress(progress);
    }
}

void MessageLine::responseLocalFile(const QString& messageId, const QString& path)
{
    Index::const_iterator itr = messageIndex.find(messageId);
    if (itr == messageIndex.end()) {
        
    } else {
        Index::const_iterator uItr = uploading.find(messageId);
        if (path.size() > 0) {
            Index::const_iterator dItr = downloading.find(messageId);
            if (dItr != downloading.end()) {
                downloading.erase(dItr);
                itr->second->showFile(path);
            } else {
                if (uItr != uploading.end()) {
                    uploading.erase(uItr);
                    std::map<QString, QString>::const_iterator muItr = uploadPaths.find(messageId);
                    if (muItr != uploadPaths.end()) {
                        uploadPaths.erase(muItr);
                    }
                    if (room) {
                        removeMessage(messageId);
                    } else {
                        Shared::Message msg = itr->second->getMessage();
                        removeMessage(messageId);
                        msg.setCurrentTime();
                        message(msg);
                        itr = messageIndex.find(messageId);
                        itr->second->showFile(path);
                    }
                } else {
                    itr->second->showFile(path); //then it is already cached file
                }
            }
        } else {
            if (uItr == uploading.end()) {
                const Shared::Message& msg = itr->second->getMessage();
                itr->second->addButton(QIcon::fromTheme("download"), tr("Download"), "<a href=\"" + msg.getOutOfBandUrl() + "\">" + msg.getOutOfBandUrl() + "</a>");
                itr->second->showComment(tr("Push the button to daownload the file"));
            } else {
                qDebug() << "An unhandled state for file uploading - empty path";
            }
        }
    }
}

void MessageLine::removeMessage(const QString& messageId)
{
    Index::const_iterator itr = messageIndex.find(messageId);
    if (itr != messageIndex.end()) {
        Message* ui = itr->second;
        const Shared::Message& msg = ui->getMessage();
        messageIndex.erase(itr);
        Order::const_iterator oItr = messageOrder.find(msg.getTime());
        if (oItr != messageOrder.end()) {
            messageOrder.erase(oItr);
        } else {
            qDebug() << "An attempt to remove message from messageLine, but it wasn't found in order";
        }
        if (msg.getOutgoing()) {
            Index::const_iterator mItr = myMessages.find(messageId);
            if (mItr != myMessages.end()) {
                myMessages.erase(mItr);
            } else {
                qDebug() << "Error removing message: it seems to be outgoing yet it wasn't found in outgoing messages";
            }
        } else {
            if (room) {
            
            } else {
                QString jid = msg.getFromJid();
                std::map<QString, Index>::iterator pItr = palMessages.find(jid);
                if (pItr != palMessages.end()) {
                    Index& pMsgs = pItr->second;
                    Index::const_iterator pmitr = pMsgs.find(messageId);
                    if (pmitr != pMsgs.end()) {
                        pMsgs.erase(pmitr);
                    } else {
                        qDebug() << "Error removing message: it seems to be incoming yet it wasn't found among messages from that penpal";
                    }
                }
            }
        }
        ui->deleteLater();
        qDebug() << "message" << messageId << "has been removed";
    } else {
        qDebug() << "An attempt to remove non existing message from messageLine";
    }
}

void MessageLine::fileError(const QString& messageId, const QString& error)
{
    Index::const_iterator itr = downloading.find(messageId);
    if (itr == downloading.end()) {
        Index::const_iterator itr = uploading.find(messageId);
        if (itr == uploading.end()) {
            //TODO may be some logging, that's not normal
        } else {
            itr->second->showComment(tr("Error uploading file: %1\nYou can try again").arg(QCoreApplication::translate("NetworkErrors", error.toLatin1())), true);
            itr->second->addButton(QIcon::fromTheme("upload"), tr("Upload"));
        }
    } else {
        const Shared::Message& msg = itr->second->getMessage();
        itr->second->addButton(QIcon::fromTheme("download"), tr("Download"), "<a href=\"" + msg.getOutOfBandUrl() + "\">" + msg.getOutOfBandUrl() + "</a>");
        itr->second->showComment(tr("Error downloading file: %1\nYou can try again").arg(QCoreApplication::translate("NetworkErrors", error.toLatin1())), true);
    }
}

void MessageLine::appendMessageWithUpload(const Shared::Message& msg, const QString& path)
{
    message(msg, true);
    QString id = msg.getId();
    Message* ui = messageIndex.find(id)->second;
    connect(ui, &Message::buttonClicked, this, &MessageLine::onUpload);     //this is in case of retry;
    ui->setProgress(0);
    ui->showComment(tr("Uploading..."));
    uploading.insert(std::make_pair(id, ui));
    uploadPaths.insert(std::make_pair(id, path));
    emit uploadFile(msg, path);
}

void MessageLine::onUpload()
{
    //TODO retry
}

void MessageLine::setMyAvatarPath(const QString& p_path)
{
    if (myAvatarPath != p_path) {
        myAvatarPath = p_path;
        for (std::pair<QString, Message*> pair : myMessages) {
            pair.second->setAvatarPath(myAvatarPath);
        }
    }
}
