#ifndef MODELS_ITEM_H
#define MODELS_ITEM_H

#include <QMap>
#include <QString>
#include <QVariant>

#include <deque>

namespace Models {

class Item : public QObject{
    Q_OBJECT
    public:
        enum Type {
            account,
            group,
            contact,
            conversation,
            root
        };
        
        explicit Item(Type p_type, const QMap<QString, QVariant> &data, Item *parentItem = 0);
        ~Item();
        
    signals:
        void changed(int col);
    
    public:
        void appendChild(Item *child);
        QString getName() const;
        void setName(const QString& name);
        
        Item *child(int row);
        int childCount() const;
        virtual int columnCount() const;
        virtual QVariant data(int column) const;
        int row() const;
        Item *parentItem();
        
        const Type type;
        
    protected:
        QString name;
        std::deque<Item*> childItems;
        Item* parent;
    };

}

#endif // MODELS_ITEM_H
