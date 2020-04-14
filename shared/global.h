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

#ifndef SHARED_GLOBAL_H
#define SHARED_GLOBAL_H

#include "enums.h"
#include "message.h"
#include "exception.h"

#include <map>
#include <set>
#include <deque>

#include <QCoreApplication>
#include <QDebug>

namespace Shared {
    
    class Global {
        Q_DECLARE_TR_FUNCTIONS(Global)
        
    public:
        Global();
        
        static Global* getInstance();
        static QString getName(Availability av);
        static QString getName(ConnectionState cs);
        static QString getName(SubscriptionState ss);
        static QString getName(Affiliation af);
        static QString getName(Role rl);
        static QString getName(Message::State rl);
        static QString getName(AccountPassword ap);
        
        static QString getDescription(AccountPassword ap);
        
        const std::deque<QString> availability;
        const std::deque<QString> connectionState;
        const std::deque<QString> subscriptionState;
        const std::deque<QString> affiliation;
        const std::deque<QString> role;
        const std::deque<QString> messageState;
        const std::deque<QString> accountPassword;
        
        const std::deque<QString> accountPasswordDescription;
        
        static bool supported(const QString& pluginName);
        static void setSupported(const QString& pluginName, bool support);
        
        static const std::set<QString> supportedImagesExts;
        
        template<typename T>
        static T fromInt(int src);
        
        template<typename T>
        static T fromInt(unsigned int src);
        
        class EnumOutOfRange: 
        public Utils::Exception
        {
        public:
            EnumOutOfRange(const std::string& p_name):Exception(), name(p_name) {}
            
            std::string getMessage() const{
                return "An attempt to get enum " + name + " from integer out of range of that enum";
            }
        private:
            std::string name;
        };
        
    private:
        static Global* instance;
        
        std::map<QString, bool> pluginSupport;
    };
}

#endif // SHARED_GLOBAL_H
