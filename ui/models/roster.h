#ifndef MODELS_ROSTER_H
#define MODELS_ROSTER_H

#include <qabstractitemmodel.h>
#include <deque>
#include <map>
#include <QVector>
#include "../../global.h"
#include "accounts.h"
#include "item.h"
#include "account.h"
#include "contact.h"

namespace Models
{

class Roster : public QAbstractItemModel
{
    class ElId;
    Q_OBJECT
public:
    Roster(QObject* parent = 0);
    ~Roster();

    void addAccount(const QMap<QString, QVariant> &data);
    void updateAccount(const QString& account, const QString& field, const QVariant& value);
    void addGroup(const QString& account, const QString& name);
    void removeGroup(const QString& account, const QString& name);
    void addContact(const QString& account, const QString& jid, const QString& name, const QString& group);
    void removeContact(const QString& account, const QString& jid, const QString& group);
    void removeContact(const QString& account, const QString& jid);
    void changeContact(const QString& account, const QString& jid, const QString& name);
    
    QVariant data ( const QModelIndex& index, int role ) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    int columnCount ( const QModelIndex& parent ) const override;
    int rowCount ( const QModelIndex& parent ) const override;
    QModelIndex parent ( const QModelIndex& child ) const override;
    QModelIndex index ( int row, int column, const QModelIndex& parent ) const override;
    
    Accounts* accountsModel;
    
private:
    Item* root;
    std::map<QString, Account*> accounts;
    std::map<ElId, Item*> groups;
    std::multimap<ElId, Contact*> contacts;
    
private slots:
    void onAccountDataChanged(const QModelIndex& tl, const QModelIndex& br, const QVector<int>& roles);
    
private:
    class ElId {
    public:
        ElId (const QString& p_account, const QString& p_name);
        
        const QString account;
        const QString name;
        
        bool operator < (const ElId& other) const;
    };
};

}

#endif // MODELS_ROSTER_H
