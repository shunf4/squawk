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

#include "vcard.h"

Shared::VCard::Contact::Contact(Shared::VCard::Contact::Role p_role, bool p_prefered):
    role(p_role),
    prefered(p_prefered) {}

Shared::VCard::Email::Email(const QString& addr, Shared::VCard::Contact::Role p_role, bool p_prefered):
    Contact(p_role, p_prefered),
    address(addr) {}

Shared::VCard::Phone::Phone(const QString& nmbr, Shared::VCard::Phone::Type p_type, Shared::VCard::Contact::Role p_role, bool p_prefered):
    Contact(p_role, p_prefered),
    number(nmbr),
    type(p_type) {}

Shared::VCard::Address::Address(const QString& zCode, const QString& cntry, const QString& rgn, const QString& lclty, const QString& strt, const QString& ext, Shared::VCard::Contact::Role p_role, bool p_prefered):
    Contact(p_role, p_prefered),
    zipCode(zCode),
    country(cntry),
    region(rgn),
    locality(lclty),
    street(strt),
    external(ext) {}

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
    receivingTime(QDateTime::currentDateTimeUtc()),
    emails(),
    phones(),
    addresses() {}

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
    addresses() {}

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

