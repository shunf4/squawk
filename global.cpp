#include "global.h"
#include <uuid/uuid.h>
#include <QApplication>
#include <QPalette>
#include <QIcon>

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
    forwarded(false)
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
    thread(),
    type(Message::normal),
    outgoing(false),
    forwarded(false)
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

Shared::Message::Type Shared::Message::getType() const
{
    return type;
}

void Shared::Message::setType(Shared::Message::Type t)
{
    type = t;
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
    quint8 t = type;
    data << t;
    data << outgoing;
    data << forwarded;
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
}

QString Shared::generateUUID()
{
    uuid_t uuid;
    uuid_generate(uuid);
    
    char uuid_str[36];
    uuid_unparse_lower(uuid, uuid_str);
    return uuid_str;
}

void Shared::Message::setCurrentTime()
{
    time = QDateTime::currentDateTime();
}

QIcon Shared::availabilityIcon(Shared::Availability av, bool big)
{
    const std::deque<QString>& fallback = QApplication::palette().window().color().lightnessF() > 0.5 ? 
    big ? 
    Shared::fallbackAvailabilityThemeIconsDarkBig:
    Shared::fallbackAvailabilityThemeIconsDarkSmall:
    big ? 
    Shared::fallbackAvailabilityThemeIconsLightBig:
    Shared::fallbackAvailabilityThemeIconsLightSmall;
    
    return QIcon::fromTheme(availabilityThemeIcons[av], QIcon(fallback[av]));
}

QIcon Shared::subscriptionStateIcon(Shared::SubscriptionState ss, bool big)
{
    const std::deque<QString>& fallback = QApplication::palette().window().color().lightnessF() > 0.5 ? 
    big ? 
    Shared::fallbackSubscriptionStateThemeIconsDarkBig:
    Shared::fallbackSubscriptionStateThemeIconsDarkSmall:
    big ? 
    Shared::fallbackSubscriptionStateThemeIconsLightBig:
    Shared::fallbackSubscriptionStateThemeIconsLightSmall;
    
    return QIcon::fromTheme(subscriptionStateThemeIcons[ss], QIcon(fallback[ss]));
}

