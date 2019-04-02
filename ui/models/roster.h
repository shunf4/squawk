#ifndef MODELS_ROSTER_H
#define MODELS_ROSTER_H

#include <qabstractitemmodel.h>
#include <deque>
#include <map>
#include "../../global.h"

namespace Models
{

class Roster : public QAbstractItemModel
{
    class Item;
    class ElId;
    class Account;
    Q_OBJECT
public:
    Roster(QObject* parent = 0);
    ~Roster();

    void addAccount(const QMap<QString, QVariant> &data);
    void updateAccount(const QString& account, const QString& field, const QVariant& value);
    
    QVariant data ( const QModelIndex& index, int role ) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    int columnCount ( const QModelIndex& parent ) const override;
    int rowCount ( const QModelIndex& parent ) const override;
    QModelIndex parent ( const QModelIndex& child ) const override;
    QModelIndex index ( int row, int column, const QModelIndex& parent ) const override;
    
private:
    Item* root;
    std::map<QString, Account*> accounts;
    std::map<ElId, Item*> elements;
    
private:
    class Item {
    public:
        enum Type {
            account,
            group,
            contect,
            conversation,
            root
        };
        
        explicit Item(Type p_type, const QMap<QString, QVariant> &data, Item *parentItem = 0);
        ~Item();
        
        void appendChild(Item *child);
        QString name() const;
        void setName(const QString& name);
        
        Item *child(int row);
        int childCount() const;
        int columnCount() const;
        QVariant data(int column) const;
        int row() const;
        Item *parentItem();
        
        const Type type;
        
    protected:
        std::deque<Item*> childItems;
        std::deque<QVariant> itemData;
        Item* parent;
    };
    
    class Account : public Item {
    public:
        explicit Account(const QMap<QString, QVariant> &data, Item *parentItem = 0);
        ~Account();
        
        void setState(int state);
    };
    
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
