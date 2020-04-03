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

#include "icons.h"

#include <QApplication>
#include <QPalette>
#include <QDebug>

QIcon Shared::availabilityIcon(Shared::Availability av, bool big)
{
    const std::deque<QString>& fallback = QApplication::palette().window().color().lightnessF() > 0.5 ? 
    big ? 
    Shared::fallbackAvailabilityThemeIconsDarkBig:
    Shared::fallbackAvailabilityThemeIconsDarkSmall:
    big ? 
    Shared::fallbackAvailabilityThemeIconsLightBig:
    Shared::fallbackAvailabilityThemeIconsLightSmall;
    
    return QIcon::fromTheme(availabilityThemeIcons[static_cast<int>(av)], QIcon(fallback[static_cast<int>(av)]));
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
    
    return QIcon::fromTheme(subscriptionStateThemeIcons[static_cast<int>(ss)], QIcon(fallback[static_cast<int>(ss)]));
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
    
    return QIcon::fromTheme(connectionStateThemeIcons[static_cast<int>(cs)], QIcon(fallback[static_cast<int>(cs)]));
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
