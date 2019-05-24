#ifndef MODELS_ACCOUNT_H
#define MODELS_ACCOUNT_H

#include "../../global.h"
#include "item.h"
#include <QVariant>
#include <QIcon>

namespace Models {
    class Account : public Item {
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
        
        void setAvailability(Shared::Availability p_avail);
        void setAvailability(unsigned int p_avail);
        Shared::Availability getAvailability() const;
        
        QIcon getStatusIcon() const;
        
        QVariant data(int column) const override;
        int columnCount() const override;
        
        void update(const QString& field, const QVariant& value);
        
    private:
        QString login;
        QString password;
        QString server;
        QString resource;
        QString error;
        Shared::ConnectionState state;
        Shared::Availability availability;
    };

}

#endif // MODELS_ACCOUNT_H
