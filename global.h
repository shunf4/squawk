#ifndef GLOBAL_H
#define GLOBAL_H

#include <QString>
#include <deque>

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

static const Availability availabilityHighest = offline;
static const Availability availabilityLowest = online;

static const std::deque<QString> ConnectionStateNames = {"Disconnected", "Connecting", "Connected", "Error"};
static const std::deque<QString> ConnectionStateThemeIcons = {"network-disconnect", "view-refresh", "network-connect", "state-error"};

};

#endif // GLOBAL_H
