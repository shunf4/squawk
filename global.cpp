#include "global.h"

Shared::Message::Message(Shared::Message::Type p_type):
    jFrom(),
    rFrom(),
    jTo(),
    rTo(),
    id(),
    body(),
    time(),
    type(p_type),
    outgoing(false)
{
}

Shared::Message::Message():
    jFrom(),
    rFrom(),
    jTo(),
    rTo(),
    id(),
    body(),
    time(),
    type(Message::normal),
    outgoing(false)
{
}

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
    return id;
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
        jFrom = from;
    } else {
        jFrom = list.front();
        rFrom = list.back();
    }
}

void Shared::Message::setTo(const QString& to)
{
    QStringList list = to.split("/");
    if (list.size() == 1) {
        jTo = to;
    } else {
        jTo = list.front();
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

void Shared::Message::setFromJid(const QString& from)
{
    jFrom = from;
}

void Shared::Message::setFromResource(const QString& from)
{
    rFrom = from;
}

void Shared::Message::setToJid(const QString& to)
{
    jTo = to;
}

void Shared::Message::setToResource(const QString& to)
{
    rTo = to;
}

bool Shared::Message::getOutgoing() const
{
    return outgoing;
}

void Shared::Message::setOutgoing(bool og)
{
    outgoing = og;
}

