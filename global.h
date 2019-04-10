#ifndef GLOBAL_H
#define GLOBAL_H

#include <QString>
#include <deque>
#include <QDateTime>

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

class Message {
public:
    Message();
    
private:
    QString jFrom;
    QString rFrom;
    QString jTo;
    QString rTo;
    QDateTime time;
};

};

#endif // GLOBAL_H
