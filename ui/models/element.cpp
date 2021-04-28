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

#include "element.h"
#include "account.h"

#include <QDebug>

Models::Element::Element(Type p_type, const Models::Account* acc, const QString& p_jid, const QMap<QString, QVariant>& data, Models::Item* parentItem):
    Item(p_type, data, parentItem),
    jid(p_jid),
    avatarPath(),
    avatarState(Shared::Avatar::empty),
    account(acc),
    feed(new MessageFeed(this))
{
    connect(feed, &MessageFeed::requestArchive, this, &Element::requestArchive);
    connect(feed, &MessageFeed::fileDownloadRequest, this, &Element::fileDownloadRequest);
    connect(feed, &MessageFeed::unreadMessagesCountChanged, this, &Element::onFeedUnreadMessagesCountChanged);
    connect(feed, &MessageFeed::unnoticedMessage, this, &Element::onFeedUnnoticedMessage);
    connect(feed, &MessageFeed::localPathInvalid, this, &Element::localPathInvalid);
    
    QMap<QString, QVariant>::const_iterator itr = data.find("avatarState");
    if (itr != data.end()) {
        setAvatarState(itr.value().toUInt());
    }
    itr = data.find("avatarPath");
    if (itr != data.end()) {
        setAvatarPath(itr.value().toString());
    }
}

Models::Element::~Element()
{
    delete feed;
}


QString Models::Element::getJid() const
{
    return jid;
}

void Models::Element::setJid(const QString& p_jid)
{
    if (jid != p_jid) {
        jid = p_jid;
        changed(1);
    }
}

void Models::Element::update(const QString& field, const QVariant& value)
{
    if (field == "jid") {
        setJid(value.toString());
    } else if (field == "avatarState") {
        setAvatarState(value.toUInt());
    } else if (field == "avatarPath") {
        setAvatarPath(value.toString());
    }
}

QString Models::Element::getAvatarPath() const
{
    return avatarPath;
}

Shared::Avatar Models::Element::getAvatarState() const
{
    return avatarState;
}

void Models::Element::setAvatarPath(const QString& path)
{
    if (path != avatarPath) {
        avatarPath = path;
        if (type == contact) {
            changed(7);
        } else if (type == room) {
            changed(8);
        }
    }
}

void Models::Element::setAvatarState(Shared::Avatar p_state)
{
    if (avatarState != p_state) {
        avatarState = p_state;
        if (type == contact) {
            changed(6);
        } else if (type == room) {
            changed(7);
        }
    }
}

void Models::Element::setAvatarState(unsigned int p_state)
{
    if (p_state <= static_cast<quint8>(Shared::Avatar::valid)) {
        Shared::Avatar state = static_cast<Shared::Avatar>(p_state);
        setAvatarState(state);
    } else {
        qDebug() << "An attempt to set invalid avatar state" << p_state << "to the element" << jid << ", skipping";
    }
}

bool Models::Element::columnInvolvedInDisplay(int col)
{
    return Item::columnInvolvedInDisplay(col) && col == 1;
}

const Models::Account * Models::Element::getParentAccount() const
{
    return account;
}

unsigned int Models::Element::getMessagesCount() const
{
    return feed->unreadMessagesCount();
}

void Models::Element::addMessage(const Shared::Message& data)
{
    feed->addMessage(data);
}

void Models::Element::changeMessage(const QString& id, const QMap<QString, QVariant>& data)
{
    feed->changeMessage(id, data);
}

void Models::Element::responseArchive(const std::list<Shared::Message> list, bool last)
{
    feed->responseArchive(list, last);
}

bool Models::Element::isRoom() const
{
    return type != contact;
}

void Models::Element::fileProgress(const QString& messageId, qreal value, bool up)
{
    feed->fileProgress(messageId, value, up);
}

void Models::Element::fileComplete(const QString& messageId, bool up)
{
    feed->fileComplete(messageId, up);
}

void Models::Element::fileError(const QString& messageId, const QString& error, bool up)
{
    feed->fileError(messageId, error, up);
}

void Models::Element::onFeedUnreadMessagesCountChanged()
{
    if (type == contact) {
        changed(4);
    } else if (type == room) {
        changed(5);
    }
}

void Models::Element::onFeedUnnoticedMessage(const Shared::Message& msg)
{
    emit unnoticedMessage(getAccountName(), msg);
}
