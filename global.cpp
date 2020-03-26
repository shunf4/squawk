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

#include "global.h"
#include <uuid/uuid.h>
#include <QApplication>
#include <QPalette>
#include <QIcon>
#include <QDebug>

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
    edited(false)
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
    forwarded(false),
    state(State::delivered),
    edited(false)
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
}

bool Shared::Message::change(const QMap<QString, QVariant>& data)
{
    QMap<QString, QVariant>::const_iterator itr = data.find("state");
    if (itr != data.end()) {
        setState(static_cast<State>(itr.value().toUInt()));
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
    itr = data.find("body");
    if (itr != data.end()) {
        setBody(itr.value().toString());
        setEdited(true);
    }
    
    return idChanged;
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

Shared::VCard::Contact::Contact(Shared::VCard::Contact::Role p_role, bool p_prefered):
    role(p_role),
    prefered(p_prefered)
{}

Shared::VCard::Email::Email(const QString& addr, Shared::VCard::Contact::Role p_role, bool p_prefered):
    Contact(p_role, p_prefered),
    address(addr)
{}

Shared::VCard::Phone::Phone(const QString& nmbr, Shared::VCard::Phone::Type p_type, Shared::VCard::Contact::Role p_role, bool p_prefered):
    Contact(p_role, p_prefered),
    number(nmbr),
    type(p_type)
{}

Shared::VCard::Address::Address(const QString& zCode, const QString& cntry, const QString& rgn, const QString& lclty, const QString& strt, const QString& ext, Shared::VCard::Contact::Role p_role, bool p_prefered):
    Contact(p_role, p_prefered),
    zipCode(zCode),
    country(cntry),
    region(rgn),
    locality(lclty),
    street(strt),
    external(ext)
{}

Shared::VCard::VCard():
    fullName(),
    firstName(),
    middleName(),
    lastName(),
    nickName(),
    description(),
    url(),
    organizationName(),
    organizationUnit(),
    organizationRole(),
    jobTitle(),
    birthday(),
    photoType(Avatar::empty),
    photoPath(),
    receivingTime(QDateTime::currentDateTime()),
    emails(),
    phones(),
    addresses()
{}

Shared::VCard::VCard(const QDateTime& creationTime):
    fullName(),
    firstName(),
    middleName(),
    lastName(),
    nickName(),
    description(),
    url(),
    organizationName(),
    organizationUnit(),
    organizationRole(),
    jobTitle(),
    birthday(),
    photoType(Avatar::empty),
    photoPath(),
    receivingTime(creationTime),
    emails(),
    phones(),
    addresses()
{
}

QString Shared::VCard::getAvatarPath() const
{
    return photoPath;
}

Shared::Avatar Shared::VCard::getAvatarType() const
{
    return photoType;
}

QDate Shared::VCard::getBirthday() const
{
    return birthday;
}

QString Shared::VCard::getDescription() const
{
    return description;
}

QString Shared::VCard::getFirstName() const
{
    return firstName;
}

QString Shared::VCard::getLastName() const
{
    return lastName;
}

QString Shared::VCard::getMiddleName() const
{
    return middleName;
}

QString Shared::VCard::getNickName() const
{
    return nickName;
}

void Shared::VCard::setAvatarPath(const QString& path)
{
    if (path != photoPath) {
        photoPath = path;
    }
}

void Shared::VCard::setAvatarType(Shared::Avatar type)
{
    if (photoType != type) {
        photoType = type;
    }
}

void Shared::VCard::setBirthday(const QDate& date)
{
    if (date.isValid() && birthday != date) {
        birthday = date;
    }
}

void Shared::VCard::setDescription(const QString& descr)
{
    if (description != descr) {
        description = descr;
    }
}

void Shared::VCard::setFirstName(const QString& first)
{
    if (firstName != first) {
        firstName = first;
    }
}

void Shared::VCard::setLastName(const QString& last)
{
    if (lastName != last) {
        lastName = last;
    }
}

void Shared::VCard::setMiddleName(const QString& middle)
{
    if (middleName != middle) {
        middleName = middle;
    }
}

void Shared::VCard::setNickName(const QString& nick)
{
    if (nickName != nick) {
        nickName = nick;
    }
}

QString Shared::VCard::getFullName() const
{
    return fullName;
}

QString Shared::VCard::getUrl() const
{
    return url;
}

void Shared::VCard::setFullName(const QString& name)
{
    if (fullName != name) {
        fullName = name;
    }
}

void Shared::VCard::setUrl(const QString& u)
{
    if (url != u) {
        url = u;
    }
}

QString Shared::VCard::getOrgName() const
{
    return organizationName;
}

QString Shared::VCard::getOrgRole() const
{
    return organizationRole;
}

QString Shared::VCard::getOrgTitle() const
{
    return jobTitle;
}

QString Shared::VCard::getOrgUnit() const
{
    return organizationUnit;
}

void Shared::VCard::setOrgName(const QString& name)
{
    if (organizationName != name) {
        organizationName = name;
    }
}

void Shared::VCard::setOrgRole(const QString& role)
{
    if (organizationRole != role) {
        organizationRole = role;
    }
}

void Shared::VCard::setOrgTitle(const QString& title)
{
    if (jobTitle != title) {
        jobTitle = title;
    }
}

void Shared::VCard::setOrgUnit(const QString& unit)
{
    if (organizationUnit != unit) {
        organizationUnit = unit;
    }
}

QDateTime Shared::VCard::getReceivingTime() const
{
    return receivingTime;
}

std::deque<Shared::VCard::Email> & Shared::VCard::getEmails()
{
    return emails;
}

std::deque<Shared::VCard::Address> & Shared::VCard::getAddresses()
{
    return addresses;
}

std::deque<Shared::VCard::Phone> & Shared::VCard::getPhones()
{
    return phones;
}

const std::deque<Shared::VCard::Email> & Shared::VCard::getEmails() const
{
    return emails;
}

const std::deque<Shared::VCard::Address> & Shared::VCard::getAddresses() const
{
    return addresses;
}

const std::deque<Shared::VCard::Phone> & Shared::VCard::getPhones() const
{
    return phones;
}

const std::deque<QString>Shared::VCard::Contact::roleNames = {"Not specified", "Personal", "Business"};
const std::deque<QString>Shared::VCard::Phone::typeNames = {"Fax", "Pager", "Voice", "Cell", "Video", "Modem", "Other"};

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

QIcon Shared::connectionStateIcon(Shared::ConnectionState cs, bool big)
{
    const std::deque<QString>& fallback = QApplication::palette().window().color().lightnessF() > 0.5 ? 
    big ? 
    Shared::fallbackConnectionStateThemeIconsDarkBig:
    Shared::fallbackConnectionStateThemeIconsDarkSmall:
    big ? 
    Shared::fallbackConnectionStateThemeIconsLightBig:
    Shared::fallbackConnectionStateThemeIconsLightSmall;
    
    return QIcon::fromTheme(connectionStateThemeIcons[cs], QIcon(fallback[cs]));
}

static const QString ds = ":images/fallback/dark/small/";
static const QString db = ":images/fallback/dark/big/";
static const QString ls = ":images/fallback/light/small/";
static const QString lb = ":images/fallback/light/big/";

QIcon Shared::icon(const QString& name, bool big)
{
    std::map<QString, std::pair<QString, QString>>::const_iterator itr = icons.find(name);
    if (itr != icons.end()) {
        const QString& prefix = QApplication::palette().window().color().lightnessF() > 0.5 ? 
            big ? db : ds:
            big ? lb : ls;
        return QIcon::fromTheme(itr->second.first, QIcon(prefix + itr->second.second + ".svg"));
    } else {
        qDebug() << "Icon" << name << "not found";
        return QIcon::fromTheme(name);
    }
}


QString Shared::iconPath(const QString& name, bool big)
{
    QString result = "";
    std::map<QString, std::pair<QString, QString>>::const_iterator itr = icons.find(name);
    if (itr != icons.end()) {
        const QString& prefix = QApplication::palette().window().color().lightnessF() > 0.5 ? 
            big ? db : ds:
            big ? lb : ls;
        result = prefix + itr->second.second + ".svg";
    }
    
    return result;
}
