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

#ifndef SHARED_ICONS_H
#define SHARED_ICONS_H

#include <QIcon>

#include <map>

#include "enums.h"

namespace Shared {

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
    {"folder", {"folder", "folder"}},
    {"document-preview", {"document-preview", "document-preview"}}
};
}

#endif // SHARED_ICONS_H
