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

#include "message.h"
#include "utils.h"

Shared::Message::Message(Shared::Message::Type p_type):
    jFrom(),
    rFrom(),
    jTo(),
    rTo(),
    id(),
    body(),
    time(),
    thread(),
    type(p_type),
    outgoing(false),
    forwarded(false),
    state(State::delivered),
    edited(false),
    errorText(),
    originalMessage(),
    lastModified(),
    stanzaId(),
    attachPath()
    {}

Shared::Message::Message():
    jFrom(),
    rFrom(),
    jTo(),
    rTo(),
    id(),
    body(),
    time(),
    thread(),
    type(Message::normal),
    outgoing(false),
    forwarded(false),
    state(State::delivered),
    edited(false),
    errorText(),
    originalMessage(),
    lastModified(),
    stanzaId(),
    attachPath()
    {}

QString Shared::Message::getBody() const
{
    return body;
}

QString Shared::Message::getFrom() const
{
    QString from = jFrom;
    if (rFrom.size() > 0) {
        from += "/" + rFrom;
    }
    return from;
}

QString Shared::Message::getTo() const
{
    QString to = jTo;
    if (rTo.size() > 0) {
        to += "/" + rTo;
    }
    return to;
}

QString Shared::Message::getId() const
{
    if (id.size() > 0) {
        return id;
    } else {
        return stanzaId;
    }
}

QDateTime Shared::Message::getTime() const
{
    return time;
}

void Shared::Message::setBody(const QString& p_body)
{
    body = p_body;
}

void Shared::Message::setFrom(const QString& from)
{
    QStringList list = from.split("/");
    if (list.size() == 1) {
        jFrom = from.toLower();
    } else {
        jFrom = list.front().toLower();
        rFrom = list.back();
    }
}

void Shared::Message::setTo(const QString& to)
{
    QStringList list = to.split("/");
    if (list.size() == 1) {
        jTo = to.toLower();
    } else {
        jTo = list.front().toLower();
        rTo = list.back();
    }
}

void Shared::Message::setId(const QString& p_id)
{
    id = p_id;
}

void Shared::Message::setTime(const QDateTime& p_time)
{
    time = p_time;
}

QString Shared::Message::getFromJid() const
{
    return jFrom;
}

QString Shared::Message::getFromResource() const
{
    return rFrom;
}

QString Shared::Message::getToJid() const
{
    return jTo;
}

QString Shared::Message::getToResource() const
{
    return rTo;
}

QString Shared::Message::getErrorText() const
{
    return errorText;
}

QString Shared::Message::getPenPalJid() const
{
    if (outgoing) {
        return jTo;
    } else {
        return jFrom;
    }
}

QString Shared::Message::getPenPalResource() const
{
    if (outgoing) {
        return rTo;
    } else {
        return rFrom;
    }
}

Shared::Message::State Shared::Message::getState() const
{
    return state;
}

bool Shared::Message::getEdited() const
{
    return edited;
}

void Shared::Message::setFromJid(const QString& from)
{
    jFrom = from.toLower();
}

void Shared::Message::setFromResource(const QString& from)
{
    rFrom = from;
}

void Shared::Message::setToJid(const QString& to)
{
    jTo = to.toLower();
}

void Shared::Message::setToResource(const QString& to)
{
    rTo = to;
}

void Shared::Message::setErrorText(const QString& err)
{
    if (state == State::error) {
        errorText = err;
    }
}

bool Shared::Message::getOutgoing() const
{
    return outgoing;
}

void Shared::Message::setOutgoing(bool og)
{
    outgoing = og;
}

bool Shared::Message::getForwarded() const
{
    return forwarded;
}

void Shared::Message::generateRandomId()
{
    id = generateUUID();
}

QString Shared::Message::getThread() const
{
    return thread;
}

void Shared::Message::setForwarded(bool fwd)
{
    forwarded = fwd;
}

void Shared::Message::setThread(const QString& p_body)
{
    thread = p_body;
}

QDateTime Shared::Message::getLastModified() const
{
    return lastModified;
}

QString Shared::Message::getOriginalBody() const
{
    return originalMessage;
}

Shared::Message::Type Shared::Message::getType() const
{
    return type;
}

void Shared::Message::setType(Shared::Message::Type t)
{
    type = t;
}

void Shared::Message::setState(Shared::Message::State p_state)
{
    state = p_state;
    
    if (state != State::error) {
        errorText = "";
    }
}

bool Shared::Message::serverStored() const
{
    return state == State::delivered || state == State::sent;
}

void Shared::Message::setEdited(bool p_edited)
{
    edited = p_edited;
}

void Shared::Message::serialize(QDataStream& data) const
{
    data << jFrom;
    data << rFrom;
    data << jTo;
    data << rTo;
    data << id;
    data << body;
    data << time;
    data << thread;
    data << (quint8)type;
    data << outgoing;
    data << forwarded;
    data << oob;
    data << (quint8)state;
    data << edited;
    if (state == State::error) {
        data << errorText;
    }
    if (edited) {
        data << originalMessage;
        data << lastModified;
    }
    data << stanzaId;
    data << attachPath;
}

void Shared::Message::deserialize(QDataStream& data)
{
    data >> jFrom;
    data >> rFrom;
    data >> jTo;
    data >> rTo;
    data >> id;
    data >> body;
    data >> time;
    data >> thread;
    quint8 t;
    data >> t;
    type = static_cast<Type>(t);
    data >> outgoing;
    data >> forwarded;
    data >> oob;
    quint8 s;
    data >> s;
    state = static_cast<State>(s);
    data >> edited;
    if (state == State::error) {
        data >> errorText;
    }
    if (edited) {
        data >> originalMessage;
        data >> lastModified;
    }
    data >> stanzaId;
    data >> attachPath;
}

bool Shared::Message::change(const QMap<QString, QVariant>& data)
{
    QMap<QString, QVariant>::const_iterator itr = data.find("state");
    if (itr != data.end()) {
        setState(static_cast<State>(itr.value().toUInt()));
    }
    
    itr = data.find("outOfBandUrl");
    if (itr != data.end()) {
        setOutOfBandUrl(itr.value().toString());
    }
    
    itr = data.find("attachPath");
    if (itr != data.end()) {
        setAttachPath(itr.value().toString());
    }
    
    if (state == State::error) {
        itr = data.find("errorText");
        if (itr != data.end()) {
            setErrorText(itr.value().toString());
        }
    }
    
    bool idChanged = false;
    itr = data.find("id");
    if (itr != data.end()) {
        QString newId = itr.value().toString();
        if (id != newId) {
            setId(newId);
            idChanged = true;
        }
    }
    
    itr = data.find("stanzaId");
    if (itr != data.end()) {
        QString newId = itr.value().toString();
        if (stanzaId != newId) {
            setStanzaId(newId);
            if (id.size() == 0) {
                idChanged = true;
            }
        }
    }
    
    itr = data.find("body");
    if (itr != data.end()) {
        QMap<QString, QVariant>::const_iterator dItr = data.find("stamp");
        QDateTime correctionDate;
        if (dItr != data.end()) {
            correctionDate = dItr.value().toDateTime();
        } else {
            correctionDate = QDateTime::currentDateTimeUtc();      //in case there is no information about time of this correction it's applied
        }
        if (!edited || lastModified < correctionDate) {
            originalMessage = body;
            lastModified = correctionDate;
            setBody(itr.value().toString());
            setEdited(true);
        }
    }
    
    return idChanged;
}

void Shared::Message::setCurrentTime()
{
    time = QDateTime::currentDateTimeUtc();
}

QString Shared::Message::getOutOfBandUrl() const
{
    return oob;
}

bool Shared::Message::hasOutOfBandUrl() const
{
    return oob.size() > 0;
}

void Shared::Message::setOutOfBandUrl(const QString& url)
{
    oob = url;
}

bool Shared::Message::storable() const
{
    return id.size() > 0 && (body.size() > 0 || oob.size()) > 0;
}

void Shared::Message::setStanzaId(const QString& sid)
{
    stanzaId = sid;
}

QString Shared::Message::getStanzaId() const
{
    return stanzaId;
}

QString Shared::Message::getAttachPath() const
{
    return attachPath;
}

void Shared::Message::setAttachPath(const QString& path)
{
    attachPath = path;
}
