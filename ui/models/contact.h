#ifndef MODELS_CONTACT_H
#define MODELS_CONTACT_H

#include "item.h"
#include "presence.h"
#include "../../global.h"
#include <QMap>
#include <QIcon>
#include <deque>

namespace Models {
    
class Account;
class Contact : public Item
{
    Q_OBJECT
public:
    typedef std::deque<Shared::Message> Messages;
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
    QString getAccountName() const;
    QString getAccountJid() const;
    QString getAccountResource() const;
    QString getContactName() const;
    QString getStatus() const;
    
    void addMessage(const Shared::Message& data);
    unsigned int getMessagesCount() const;
    void dropMessages();
    void getMessages(Messages& container) const;
    
protected:
    void _removeChild(int index) override;
    const Account* getParentAccount() const;
    
protected slots:
    void refresh();
    void toOfflineState() override;
    
protected:
    void setAvailability(Shared::Availability p_state);
    void setAvailability(unsigned int p_state);
    void setState(Shared::SubscriptionState p_state);
    void setState(unsigned int p_state);
    void setJid(const QString p_jid);
    void setStatus(const QString& p_state);
    
private:
    QString jid;
    Shared::Availability availability;
    Shared::SubscriptionState state;
    QMap<QString, Presence*> presences;
    Messages messages;
    unsigned int childMessages;
    QString status;
};

}

#endif // MODELS_CONTACT_H
