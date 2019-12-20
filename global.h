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

#ifndef GLOBAL_H
#define GLOBAL_H

#include <QString>
#include <QCoreApplication>
#include <deque>
#include <QDateTime>
#include <QDataStream>
#include <QColor>

namespace Shared {
    
enum ConnectionState {
    disconnected,
    connecting,
    connected,
    error
};

enum Availability {
    online,
    away,
    extendedAway,
    busy,
    chatty,
    invisible,
    offline
};

enum SubscriptionState {
    none,
    from,
    to,
    both,
    unknown
};

enum class Affiliation {
    unspecified, 
    outcast, 
    nobody, 
    member, 
    admin, 
    owner 
};

enum class Role { 
    unspecified, 
    nobody, 
    visitor, 
    participant, 
    moderator 
};

enum class Avatar {
    empty,
    autocreated,
    valid
};

static const Availability availabilityHighest = offline;
static const Availability availabilityLowest = online;

static const SubscriptionState subscriptionStateHighest = unknown;
static const SubscriptionState subscriptionStateLowest = none;

static const Affiliation affiliationHighest = Affiliation::owner;
static const Affiliation affiliationLowest = Affiliation::unspecified;

static const Role roleHighest = Role::moderator;
static const Role roleLowest = Role::unspecified;

static const std::deque<QString> connectionStateNames = {"Disconnected", "Connecting", "Connected", "Error"};
static const std::deque<QString> connectionStateThemeIcons = {"state-offline", "state-sync", "state-ok", "state-error"};

static const std::deque<QString> availabilityThemeIcons = {
    "user-online",
    "user-away",
    "user-away-extended",
    "user-busy",
    "chatty",
    "user-invisible",
    "user-offline"
};
static const std::deque<QString> availabilityNames = {"Online", "Away", "Absent", "Busy", "Chatty", "Invisible", "Offline"};

static const std::deque<QString> subscriptionStateThemeIcons = {"edit-none", "arrow-down-double", "arrow-up-double", "dialog-ok", "question"};
static const std::deque<QString> subscriptionStateNames = {"None", "From", "To", "Both", "Unknown"};

static const std::deque<QString> affiliationNames = {"Unspecified", "Outcast", "Nobody", "Member", "Admin", "Owner"};
static const std::deque<QString> roleNames = {"Unspecified", "Nobody", "Visitor", "Participant", "Moderator"};
QString generateUUID();

static const std::vector<QColor> colorPalette = {
    QColor(244, 27, 63),
    QColor(21, 104, 156),
    QColor(38, 156, 98),
    QColor(247, 103, 101),
    QColor(121, 37, 117),
    QColor(242, 202, 33),
    QColor(168, 22, 63),
    QColor(35, 100, 52),
    QColor(52, 161, 152),
    QColor(239, 53, 111),
    QColor(237, 234, 36),
    QColor(153, 148, 194),
    QColor(211, 102, 151),
    QColor(194, 63, 118),
    QColor(249, 149, 51),
    QColor(244, 206, 109),
    QColor(121, 105, 153),
    QColor(244, 199, 30),
    QColor(28, 112, 28),
    QColor(172, 18, 20),
    QColor(25, 66, 110),
    QColor(25, 149, 104),
    QColor(214, 148, 0),
    QColor(203, 47, 57),
    QColor(4, 54, 84),
    QColor(116, 161, 97),
    QColor(50, 68, 52),
    QColor(237, 179, 20),
    QColor(69, 114, 147),
    QColor(242, 212, 31),
    QColor(248, 19, 20),
    QColor(84, 102, 84),
    QColor(25, 53, 122),
    QColor(91, 91, 109),
    QColor(17, 17, 80),
    QColor(54, 54, 94)
};

class Message {
public:
    enum Type {
        error,
        normal,
        chat,
        groupChat,
        headline
    };
    Message(Type p_type);
    Message();

    void setFrom(const QString& from);
    void setFromResource(const QString& from);
    void setFromJid(const QString& from);
    void setTo(const QString& to);
    void setToResource(const QString& to);
    void setToJid(const QString& to);
    void setTime(const QDateTime& p_time);
    void setId(const QString& p_id);
    void setBody(const QString& p_body);
    void setThread(const QString& p_body);
    void setOutgoing(bool og);
    void setForwarded(bool fwd);
    void setType(Type t);
    void setCurrentTime();
    void setOutOfBandUrl(const QString& url);
    
    QString getFrom() const;
    QString getFromJid() const;
    QString getFromResource() const;
    QString getTo() const;
    QString getToJid() const;
    QString getToResource() const;
    QDateTime getTime() const;
    QString getId() const;
    QString getBody() const;
    QString getThread() const;
    bool getOutgoing() const;
    bool getForwarded() const;
    Type getType() const;
    bool hasOutOfBandUrl() const;
    bool storable() const;
    QString getOutOfBandUrl() const;
    
    QString getPenPalJid() const;
    QString getPenPalResource() const;
    void generateRandomId();
    
    void serialize(QDataStream& data) const;
    void deserialize(QDataStream& data);
    
private:
    QString jFrom;
    QString rFrom;
    QString jTo;
    QString rTo;
    QString id;
    QString body;
    QDateTime time;
    QString thread;
    Type type;
    bool outgoing;
    bool forwarded;
    QString oob;
};

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

static const std::deque<QString> fallbackAvailabilityThemeIconsLightBig = {
    ":images/fallback/light/big/online.svg",
    ":images/fallback/light/big/away.svg",
    ":images/fallback/light/big/absent.svg",
    ":images/fallback/light/big/busy.svg",
    ":images/fallback/light/big/chatty.svg",
    ":images/fallback/light/big/invisible.svg",
    ":images/fallback/light/big/offline.svg"
};

static const std::deque<QString> fallbackSubscriptionStateThemeIconsLightBig = {
    ":images/fallback/light/big/edit-none.svg",
    ":images/fallback/light/big/arrow-down-double.svg",
    ":images/fallback/light/big/arrow-up-double.svg",
    ":images/fallback/light/big/dialog-ok.svg",
    ":images/fallback/light/big/question.svg"
};

static const std::deque<QString> fallbackConnectionStateThemeIconsLightBig = {
    ":images/fallback/light/big/state-offline.svg",
    ":images/fallback/light/big/state-sync.svg",
    ":images/fallback/light/big/state-ok.svg",
    ":images/fallback/light/big/state-error.svg"
};

static const std::deque<QString> fallbackAvailabilityThemeIconsLightSmall = {
    ":images/fallback/light/small/online.svg",
    ":images/fallback/light/small/away.svg",
    ":images/fallback/light/small/absent.svg",
    ":images/fallback/light/small/busy.svg",
    ":images/fallback/light/small/chatty.svg",
    ":images/fallback/light/small/invisible.svg",
    ":images/fallback/light/small/offline.svg"
};

static const std::deque<QString> fallbackSubscriptionStateThemeIconsLightSmall = {
    ":images/fallback/light/small/edit-none.svg",
    ":images/fallback/light/small/arrow-down-double.svg",
    ":images/fallback/light/small/arrow-up-double.svg",
    ":images/fallback/light/small/dialog-ok.svg",
    ":images/fallback/light/small/question.svg"
};

static const std::deque<QString> fallbackConnectionStateThemeIconsLightSmall = {
    ":images/fallback/light/small/state-offline.svg",
    ":images/fallback/light/small/state-sync.svg",
    ":images/fallback/light/small/state-ok.svg",
    ":images/fallback/light/small/state-error.svg"
};

static const std::deque<QString> fallbackAvailabilityThemeIconsDarkBig = {
    ":images/fallback/dark/big/online.svg",
    ":images/fallback/dark/big/away.svg",
    ":images/fallback/dark/big/absent.svg",
    ":images/fallback/dark/big/busy.svg",
    ":images/fallback/dark/big/chatty.svg",
    ":images/fallback/dark/big/invisible.svg",
    ":images/fallback/dark/big/offline.svg"
};

static const std::deque<QString> fallbackSubscriptionStateThemeIconsDarkBig = {
    ":images/fallback/dark/big/edit-none.svg",
    ":images/fallback/dark/big/arrow-down-double.svg",
    ":images/fallback/dark/big/arrow-up-double.svg",
    ":images/fallback/dark/big/dialog-ok.svg",
    ":images/fallback/dark/big/question.svg"
};

static const std::deque<QString> fallbackConnectionStateThemeIconsDarkBig = {
    ":images/fallback/dark/big/state-offline.svg",
    ":images/fallback/dark/big/state-sync.svg",
    ":images/fallback/dark/big/state-ok.svg",
    ":images/fallback/dark/big/state-error.svg"
};

static const std::deque<QString> fallbackAvailabilityThemeIconsDarkSmall = {
    ":images/fallback/dark/small/online.svg",
    ":images/fallback/dark/small/away.svg",
    ":images/fallback/dark/small/absent.svg",
    ":images/fallback/dark/small/busy.svg",
    ":images/fallback/dark/small/chatty.svg",
    ":images/fallback/dark/small/invisible.svg",
    ":images/fallback/dark/small/offline.svg"
};

static const std::deque<QString> fallbackSubscriptionStateThemeIconsDarkSmall = {
    ":images/fallback/dark/small/edit-none.svg",
    ":images/fallback/dark/small/arrow-down-double.svg",
    ":images/fallback/dark/small/arrow-up-double.svg",
    ":images/fallback/dark/small/dialog-ok.svg",
    ":images/fallback/dark/small/question.svg"
};

static const std::deque<QString> fallbackConnectionStateThemeIconsDarkSmall = {
    ":images/fallback/dark/small/state-offline.svg",
    ":images/fallback/dark/small/state-sync.svg",
    ":images/fallback/dark/small/state-ok.svg",
    ":images/fallback/dark/small/state-error.svg"
};

QIcon availabilityIcon(Availability av, bool big = false);
QIcon subscriptionStateIcon(SubscriptionState ss, bool big = false);
QIcon connectionStateIcon(ConnectionState cs, bool big = false);
QIcon icon(const QString& name, bool big = false);
QString iconPath(const QString& name, bool big = false);

static const std::map<QString, std::pair<QString, QString>> icons = {
    {"user-online", {"user-online", "online"}},
    {"user-away", {"user-away", "away"}},
    {"user-away-extended", {"user-away-extended", "absent"}},
    {"user-busy", {"user-busy", "busy"}},
    {"user-chatty", {"chatty", "chatty"}},
    {"user-invisible", {"user-invisible", "invisible"}},
    {"user-offline", {"offline", "offline"}},
    {"edit-none", {"edit-none", "edit-none"}}, 
    {"arrow-down-double", {"arrow-down-double", "arrow-down-double"}}, 
    {"arrow-up-double", {"arrow-up-double", "arrow-up-double"}}, 
    {"dialog-ok", {"dialog-ok", "dialog-ok"}}, 
    {"question", {"question", "question"}},
    {"state-offline", {"state-offline", "state-offline"}}, 
    {"state-sync", {"state-sync", "state-sync"}}, 
    {"state-ok", {"state-ok", "state-ok"}}, 
    {"state-error", {"state-error", "state-error"}},
    
    {"edit-copy", {"edit-copy", "copy"}},
    {"edit-delete", {"edit-delete", "edit-delete"}},
    {"edit-rename", {"edit-rename", "edit-rename"}},
    {"mail-message", {"mail-message", "mail-message"}},
    {"mail-attachment", {"mail-attachment", "mail-attachment"}},
    {"network-connect", {"network-connect", "network-connect"}},
    {"network-disconnect", {"network-disconnect", "network-disconnect"}},
    {"news-subscribe", {"news-subscribe", "news-subscribe"}},
    {"news-unsubscribe", {"news-unsubscribe", "news-unsubscribe"}},
    {"view-refresh", {"view-refresh", "view-refresh"}},
    {"send", {"document-send", "send"}},
    {"clean", {"edit-clear-all", "clean"}},
    {"user", {"user", "user"}},
    {"user-properties", {"user-properties", "user-properties"}},
    {"group", {"group", "group"}},
    {"group-new", {"resurce-group-new", "group-new"}},
    {"favorite", {"favorite", "favorite"}},
    {"unfavorite", {"draw-star", "unfavorite"}},
    {"list-add", {"list-add", "add"}},
};

};

#endif // GLOBAL_H
