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
    layout(new QVBoxLayout(this)),
    myName(),
    palNames(),
    views(),
    room(p_room),
    busyPixmap(new QGraphicsPixmapItem(Shared::icon("view-refresh", true).pixmap(70))),
    busyScene(),
    busyLabel(&busyScene),
    busyLayout(),
    busyShown(false),
    rotation()
{
    setBackgroundRole(QPalette::Base);
    layout->addStretch();
    
    busyScene.addItem(busyPixmap);
    busyLayout.addStretch();
    busyLayout.addWidget(&busyLabel);
    busyLayout.addStretch();
    busyLabel.setMaximumSize(70, 70);
    busyLabel.setMinimumSize(70, 70);
    busyLabel.setSceneRect(0, 0, 70, 70);
    busyLabel.setFrameStyle(0);
    busyLabel.setContentsMargins(0, 0, 0, 0);
    busyLabel.setInteractive(false);
    busyPixmap->setTransformOriginPoint(35, 35);
    busyPixmap->setTransformationMode(Qt::SmoothTransformation);
    busyPixmap->setOffset(0, 0);;
    
    rotation.setDuration(500);
    rotation.setStartValue(0.0f);
    rotation.setEndValue(180.0f);
    rotation.setLoopCount(-1);
    connect(&rotation, SIGNAL(valueChanged(const QVariant&)), this, SLOT(onAnimationValueChanged(const QVariant&)));
}

MessageLine::~MessageLine()
{
    for (Index::const_iterator itr = messageIndex.begin(), end = messageIndex.end(); itr != end; ++itr) {
        delete itr->second;
    }
}

MessageLine::Position MessageLine::message(const Shared::Message& msg)
{
    QString id = msg.getId();
    Index::iterator itr = messageIndex.find(id);
    if (itr != messageIndex.end()) {
        qDebug() << "received more then one message with the same id, skipping yet the new one";
        return invalid;
    }
    
    QString sender;
    bool outgoing;
    
    if (room) {
        if (msg.getFromResource() == myName) {
            sender = myName;
            outgoing = false;
        } else {
            sender = msg.getFromResource();
            outgoing = true;
        }
    } else {
        if (msg.getOutgoing()) {
            sender = myName;
            outgoing = false;
        } else {
            QString jid = msg.getFromJid();
            std::map<QString, QString>::iterator itr = palNames.find(jid);
            if (itr != palNames.end()) {
                sender = itr->second;
            } else {
                sender = jid;
            }
            outgoing = true;
        }
    }
    
    Message* message = new Message(msg, outgoing, sender);
    
    std::pair<Order::const_iterator, bool> result = messageOrder.insert(std::make_pair(msg.getTime(), message));
    if (!result.second) {
        qDebug() << "Error appending a message into a message list - seems like the time of that message exactly matches the time of some other message, can't put them in order, skipping yet";
        delete message;
        return invalid;
    }
    if (outgoing) {
        if (room) {
            
        } else {
            QString jid = msg.getFromJid();
            std::map<QString, Index>::iterator pItr = palMessages.find(jid);
            if (pItr == palMessages.end()) {
                pItr = palMessages.insert(std::make_pair(jid, Index())).first;
            }
            pItr->second.insert(std::make_pair(id, message));
        }
    } else {
        myMessages.insert(std::make_pair(id, message));
    }
    messageIndex.insert(std::make_pair(id, message));
    int index = std::distance<Order::const_iterator>(messageOrder.begin(), result.first);   //need to make with binary indexed tree
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
    
    return res;
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
        layout->insertLayout(0, &busyLayout);
        busyShown = true;
        rotation.start();
        busyLabel.show();
    }
}

void MessageLine::hideBusyIndicator()
{
    if (busyShown) {
        busyLabel.hide();
        rotation.stop();
        layout->removeItem(&busyLayout);
        busyShown = false;
    }
}

void MessageLine::onAnimationValueChanged(const QVariant& value)
{
    busyPixmap->setRotation(value.toReal());
}
