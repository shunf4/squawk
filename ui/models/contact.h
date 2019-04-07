#ifndef MODELS_CONTACT_H
#define MODELS_CONTACT_H

#include "item.h"
#include "presence.h"
#include "../../global.h"
#include <QMap>
#include <QIcon>

namespace Models {

class Contact : public Item
{
    Q_OBJECT
public:
    Contact(const QString& p_jid ,const QMap<QString, QVariant> &data, Item *parentItem = 0);
    ~Contact();
    
    QString getJid() const;
    Shared::Availability getAvailability() const;
    Shared::SubscriptionState getState() const;
    QIcon getStatusIcon() const;
    
    int columnCount() const override;
    QVariant data(int column) const override;
        
    void update(const QString& field, const QVariant& value);
    
    void addPresence(const QString& name, const QMap<QString, QVariant>& data);
    void removePresence(const QString& name);
    
    void appendChild(Models::Item * child) override;
    
protected:
    void refresh();
    void changed(int col) override;
    void _removeChild(int index) override;
    
protected:
    void setAvailability(Shared::Availability p_state);
    void setAvailability(unsigned int p_state);
    void setState(Shared::SubscriptionState p_state);
    void setState(unsigned int p_state);
    void setJid(const QString p_jid);
    
private:
    QString jid;
    Shared::Availability availability;
    Shared::SubscriptionState state;
    QMap<QString, Presence*> presences;
};

}

#endif // MODELS_CONTACT_H
