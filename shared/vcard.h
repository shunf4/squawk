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

#ifndef SHARED_VCARD_H
#define SHARED_VCARD_H

#include <QString>
#include <QDateTime>

#include <deque>

#include "enums.h"

namespace Shared {

class VCard {
    class Contact {
    public:
        enum Role {
            none,
            home,
            work
        };
        static const std::deque<QString> roleNames;
        
        Contact(Role p_role = none, bool p_prefered = false);
        
        Role role;
        bool prefered;
    };
public:
    class Email : public Contact {
    public:
        Email(const QString& address, Role p_role = none, bool p_prefered = false);
        
        QString address;
    };
    class Phone : public Contact {
    public:
        enum Type {
            fax,
            pager,
            voice,
            cell,
            video,
            modem,
            other
        };
        static const std::deque<QString> typeNames;
        Phone(const QString& number, Type p_type = voice, Role p_role = none, bool p_prefered = false);
        
        QString number;
        Type type;
    };
    class Address : public Contact {
    public:
        Address(
            const QString& zCode = "", 
            const QString& cntry = "", 
            const QString& rgn = "", 
            const QString& lclty = "", 
            const QString& strt = "", 
            const QString& ext = "", 
            Role p_role = none, 
            bool p_prefered = false
        );
        
        QString zipCode;
        QString country;
        QString region;
        QString locality;
        QString street;
        QString external;
    };
    VCard();
    VCard(const QDateTime& creationTime);
    
    QString getFullName() const;
    void setFullName(const QString& name);
    QString getFirstName() const;
    void setFirstName(const QString& first);
    QString getMiddleName() const;
    void setMiddleName(const QString& middle);
    QString getLastName() const;
    void setLastName(const QString& last);
    QString getNickName() const;
    void setNickName(const QString& nick);
    QString getDescription() const;
    void setDescription(const QString& descr);
    QString getUrl() const;
    void setUrl(const QString& u);
    QDate getBirthday() const;
    void setBirthday(const QDate& date);
    Avatar getAvatarType() const;
    void setAvatarType(Avatar type);
    QString getAvatarPath() const;
    void setAvatarPath(const QString& path);
    QString getOrgName() const;
    void setOrgName(const QString& name);
    QString getOrgUnit() const;
    void setOrgUnit(const QString& unit);
    QString getOrgRole() const;
    void setOrgRole(const QString& role);
    QString getOrgTitle() const;
    void setOrgTitle(const QString& title);
    QDateTime getReceivingTime() const;
    std::deque<Email>& getEmails();
    const std::deque<Email>& getEmails() const;
    std::deque<Phone>& getPhones();
    const std::deque<Phone>& getPhones() const;
    std::deque<Address>& getAddresses();
    const std::deque<Address>& getAddresses() const;
    
private:
    QString fullName;
    QString firstName;
    QString middleName;
    QString lastName;
    QString nickName;
    QString description;
    QString url;
    QString organizationName;
    QString organizationUnit;
    QString organizationRole;
    QString jobTitle;
    QDate birthday;
    Avatar photoType;
    QString photoPath;
    QDateTime receivingTime;
    std::deque<Email> emails;
    std::deque<Phone> phones;
    std::deque<Address> addresses;
};

}

#endif // SHARED_VCARD_H
