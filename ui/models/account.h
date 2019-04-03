#ifndef MODELS_ACCOUNT_H
#define MODELS_ACCOUNT_H

#include "../../global.h"
#include "item.h"
#include <QVariant>

namespace Models {
    class Account : public Item {
    public:
        explicit Account(const QMap<QString, QVariant> &data, Item *parentItem = 0);
        ~Account();
        
        void setState(int p_state);
        int getState() const;
        
        void setLogin(const QString& p_login);
        QString getLogin() const;
        
        void setServer(const QString& p_server);
        QString getServer() const;
        
        void setPassword(const QString& p_password);
        QString getPassword() const;
        
        QVariant data(int column) const override;
        int columnCount() const override;
        
        void update(const QString& field, const QVariant& value);
        
    private:
        QString login;
        QString password;
        QString server;
        int state;
    };

}

#endif // MODELS_ACCOUNT_H
