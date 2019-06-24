#ifndef GLOBAL_H
#define GLOBAL_H

#include <QString>
#include <deque>
#include <QDateTime>
#include <QDataStream>

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

static const Availability availabilityHighest = offline;
static const Availability availabilityLowest = online;

static const SubscriptionState subscriptionStateHighest = unknown;
static const SubscriptionState subscriptionStateLowest = none;

static const std::deque<QString> connectionStateNames = {"Disconnected", "Connecting", "Connected", "Error"};
static const std::deque<QString> connectionStateThemeIcons = {"network-disconnect", "view-refresh", "network-connect", "state-error"};

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

QString generateUUID();

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

QIcon availabilityIcon(Availability av, bool big = false);
QIcon subscriptionStateIcon(SubscriptionState ss, bool big = false);

};

#endif // GLOBAL_H
