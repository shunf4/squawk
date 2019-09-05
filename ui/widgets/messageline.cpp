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
#include <QGraphicsDropShadowEffect>

const QRegExp urlReg("^(?!<img\\ssrc=\")((?:https?|ftp)://\\S+)");
const QRegExp imgReg("((?:https?|ftp)://\\S+\\.(?:jpg|jpeg|png|svg|gif))");

MessageLine::MessageLine(bool p_room, QWidget* parent):
    QWidget(parent),
    messageIndex(),
    messageOrder(),
    layout(new QVBoxLayout()),
    myName(),
    palNames(),
    views(),
    room(p_room),
    busyLabel(),
    busyLayout(),
    busyShown(false),
    rotation()
{
    setLayout(layout);
    setBackgroundRole(QPalette::Base);
    layout->addStretch();
    
    busyLabel.setPixmap(Shared::icon("view-refresh", true).pixmap(50));
    busyLayout.addStretch();
    busyLayout.addWidget(&busyLabel);
    busyLayout.addStretch();
    
    busyLabel.hide();
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
    
    Shared::Message* copy = new Shared::Message(msg);
    std::pair<Order::const_iterator, bool> result = messageOrder.insert(std::make_pair(msg.getTime(), copy));
    if (!result.second) {
        qDebug() << "Error appending a message into a message list - seems like the time of that message exactly matches the time of some other message, can't put them in order, skipping yet";
        delete copy;
        return invalid;
    }
    messageIndex.insert(std::make_pair(id, copy));
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
    
    QVBoxLayout* vBox = new QVBoxLayout();
    QHBoxLayout* hBox = new QHBoxLayout();
    QWidget* message = new QWidget();
    message->setLayout(vBox);
    message->setBackgroundRole(QPalette::AlternateBase);
    message->setAutoFillBackground(true);
    
    
    QString bd = msg.getBody();
    //bd.replace(imgReg, "<img src=\"\\1\"/>");
    bd.replace(urlReg, "<a href=\"\\1\">\\1</a>");
    QLabel* body = new QLabel(bd);
    body->setTextInteractionFlags(body->textInteractionFlags() | Qt::TextSelectableByMouse);
    QLabel* sender = new QLabel();
    QLabel* time = new QLabel(msg.getTime().toLocalTime().toString());
    QFont dFont = time->font();
    dFont.setItalic(true);
    dFont.setPointSize(dFont.pointSize() - 2);
    time->setFont(dFont);
    time->setForegroundRole(QPalette::ToolTipText);
    QFont f;
    f.setBold(true);
    sender->setFont(f);
    
    body->setWordWrap(true);
    body->setOpenExternalLinks(true);
    
    vBox->addWidget(sender);
    vBox->addWidget(body);
    vBox->addWidget(time);
    
    QGraphicsDropShadowEffect *effect = new QGraphicsDropShadowEffect;
    effect->setBlurRadius(10);
    effect->setXOffset(1);
    effect->setYOffset(1);
    effect->setColor(Qt::black);

    message->setGraphicsEffect(effect);
    
    if (room) {
        if (msg.getFromResource() == myName) {
            //body->setAlignment(Qt::AlignRight);
            sender->setAlignment(Qt::AlignRight);
            time->setAlignment(Qt::AlignRight);
            sender->setText(myName);
            hBox->addStretch();
            hBox->addWidget(message);
        } else {
            sender->setText(msg.getFromResource());
            hBox->addWidget(message);
            hBox->addStretch();
        }
    } else {
        if (msg.getOutgoing()) {
            //body->setAlignment(Qt::AlignRight);
            sender->setAlignment(Qt::AlignRight);
            time->setAlignment(Qt::AlignRight);
            sender->setText(myName);
            hBox->addStretch();
            hBox->addWidget(message);
        } else {
            QString jid = msg.getFromJid();
            std::map<QString, QString>::iterator itr = palNames.find(jid);
            if (itr != palNames.end()) {
                sender->setText(itr->second);
            } else {
                sender->setText(jid);
            }
            hBox->addWidget(message);
            hBox->addStretch();
        }
    }
        
    if (res == end) {
        layout->addLayout(hBox);
    } else {
        layout->insertLayout(index, hBox);
    }
    
    return res;
}

void MessageLine::setMyName(const QString& name)
{
    myName = name;
}

void MessageLine::setPalName(const QString& jid, const QString& name)
{
    std::map<QString, QString>::iterator itr = palNames.find(jid);
    if (itr == palNames.end()) {
        palNames.insert(std::make_pair(jid, name));
    } else {
        itr->second = name;
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
    QTransform r;
    r.rotate(value.toReal());
    QPixmap pxm = Shared::icon("view-refresh", true).pixmap(50).transformed(r, Qt::SmoothTransformation);
    int dw = pxm.width() - 50;
    int dh = pxm.height() - 50;
    pxm = pxm.copy(dw/2, dh/2, 50, 50);
    busyLabel.setPixmap(pxm);
}
