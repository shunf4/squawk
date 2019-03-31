#ifndef MODELS_ROSTER_H
#define MODELS_ROSTER_H

#include <qabstractitemmodel.h>
#include <deque>
#include <map>

namespace Models
{

class Roster : public QAbstractItemModel
{
    class Item;
    class ElId;
    Q_OBJECT
public:
    Roster(QObject* parent = 0);
    ~Roster();

    void addAccount(const QMap<QString, QVariant> &data);
    
    QVariant data ( const QModelIndex& index, int role ) const;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    int columnCount ( const QModelIndex& parent ) const;
    int rowCount ( const QModelIndex& parent ) const;
    QModelIndex parent ( const QModelIndex& child ) const;
    QModelIndex index ( int row, int column, const QModelIndex& parent ) const;
    
private:
    Item* root;
    std::map<QString, Item*> accounts;
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
        
        Item *child(int row);
        int childCount() const;
        int columnCount() const;
        QVariant data(int column) const;
        int row() const;
        Item *parentItem();
        
        const Type type;
        
    private:
        std::deque<Item*> childItems;
        std::deque<QVariant> itemData;
        Item* parent;
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
