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
#include "group.h"
#include "room.h"

namespace Models
{

class Roster : public QAbstractItemModel
{
    Q_OBJECT
public:
    class ElId;
    Roster(QObject* parent = 0);
    ~Roster();

    void addAccount(const QMap<QString, QVariant> &data);
    void updateAccount(const QString& account, const QString& field, const QVariant& value);
    void removeAccount(const QString& account);
    void addGroup(const QString& account, const QString& name);
    void removeGroup(const QString& account, const QString& name);
    void addContact(const QString& account, const QString& jid, const QString& group, const QMap<QString, QVariant>& data);
    void removeContact(const QString& account, const QString& jid, const QString& group);
    void removeContact(const QString& account, const QString& jid);
    void changeContact(const QString& account, const QString& jid, const QMap<QString, QVariant>& data);
    void addPresence(const QString& account, const QString& jid, const QString& name, const QMap<QString, QVariant>& data);
    void removePresence(const QString& account, const QString& jid, const QString& name);
    void addMessage(const QString& account, const Shared::Message& data);
    void dropMessages(const QString& account, const QString& jid);
    void addRoom(const QString& account, const QString jid, const QMap<QString, QVariant>& data);
    void changeRoom(const QString& account, const QString jid, const QMap<QString, QVariant>& data);
    void removeRoom(const QString& account, const QString jid);
    QString getContactName(const QString& account, const QString& jid);
    
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
    std::map<ElId, Group*> groups;
    std::multimap<ElId, Contact*> contacts;
    std::map<ElId, Room*> rooms;
    
private slots:
    void onAccountDataChanged(const QModelIndex& tl, const QModelIndex& br, const QVector<int>& roles);
    void onChildChanged(Models::Item* item, int row, int col);
    void onChildIsAboutToBeInserted(Item* parent, int first, int last);
    void onChildInserted();
    void onChildIsAboutToBeRemoved(Item* parent, int first, int last);
    void onChildRemoved();
    void onChildIsAboutToBeMoved(Item* source, int first, int last, Item* destination, int newIndex);
    void onChildMoved();
    
public:
    class ElId {
    public:
        ElId (const QString& p_account = "", const QString& p_name = "");
        
        const QString account;
        const QString name;
        
        bool operator < (const ElId& other) const;
    };
};

}

#endif // MODELS_ROSTER_H
