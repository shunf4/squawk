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
    "im-user-online",
    "im-user-away",
    "im-user-away",
    "im-user-busy",
    "im-user-online",
    "im-user-offline"
};
static const std::deque<QString> availabilityNames = {"Online", "Away", "Absent", "Busy", "Chatty", "Offline"};

static const std::deque<QString> subscriptionStateThemeIcons = {"edit-none", "arrow-down-double", "arrow-up-double", "dialog-ok", "question"};

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

};

#endif // GLOBAL_H
