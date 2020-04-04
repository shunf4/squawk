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

#ifndef MODELS_ACCOUNT_H
#define MODELS_ACCOUNT_H

#include "item.h"
#include "shared/enums.h"
#include "shared/utils.h"
#include "shared/icons.h"
#include "shared/global.h"
#include <QVariant>
#include <QIcon>

namespace Models {
    class Account : public Item {
        Q_OBJECT
    public:
        explicit Account(const QMap<QString, QVariant> &data, Item *parentItem = 0);
        ~Account();
        
        void setState(unsigned int p_state);
        void setState(Shared::ConnectionState p_state);
        Shared::ConnectionState getState() const;
        
        void setLogin(const QString& p_login);
        QString getLogin() const;
        
        void setServer(const QString& p_server);
        QString getServer() const;
        
        void setPassword(const QString& p_password);
        QString getPassword() const;
        
        void setResource(const QString& p_resource);
        QString getResource() const;
        
        void setError(const QString& p_resource);
        QString getError() const;
        
        void setAvatarPath(const QString& path);
        QString getAvatarPath();
        
        void setAvailability(Shared::Availability p_avail);
        void setAvailability(unsigned int p_avail);
        Shared::Availability getAvailability() const;
        
        void setPasswordType(Shared::AccountPassword pt);
        void setPasswordType(unsigned int pt);
        Shared::AccountPassword getPasswordType() const;
        
        QIcon getStatusIcon(bool big = false) const;
        
        QVariant data(int column) const override;
        int columnCount() const override;
        
        void update(const QString& field, const QVariant& value);
        
        QString getBareJid() const;
        QString getFullJid() const;
        
    private:
        QString login;
        QString password;
        QString server;
        QString resource;
        QString error;
        QString avatarPath;
        Shared::ConnectionState state;
        Shared::Availability availability;
        Shared::AccountPassword passwordType;
        
    protected slots:
        void toOfflineState() override;
    };

}

#endif // MODELS_ACCOUNT_H
