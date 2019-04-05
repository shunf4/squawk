#ifndef MODELS_CONTACT_H
#define MODELS_CONTACT_H

#include "item.h"

namespace Models {

class Contact : public Item
{
    Q_OBJECT
public:
    Contact(const QMap<QString, QVariant> &data, Item *parentItem = 0);
    ~Contact();
    
    QString getJid() const;
    void setJid(const QString p_jid);
    
    int getState() const;
    void  setState(int p_state);
    
    int columnCount() const override;
    QVariant data(int column) const override;
        
    void update(const QString& field, const QVariant& value);
    
private:
    QString jid;
    int state;
};

}

#endif // MODELS_CONTACT_H
