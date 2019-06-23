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
            presence,
            root
        };
        
        explicit Item(Type p_type, const QMap<QString, QVariant> &data, Item *parentItem = 0);
        ~Item();
        
    signals:
        void childChanged(Models::Item* item, int row, int col);
        void childIsAboutToBeInserted(Item* parent, int first, int last);
        void childInserted();
        void childIsAboutToBeRemoved(Item* parent, int first, int last);
        void childRemoved();
        void childIsAboutToBeMoved(Item* source, int first, int last, Item* destination, int newIndex);
        void childMoved();
        
    public:
        virtual void appendChild(Item *child);
        virtual void removeChild(int index);
        QString getName() const;
        void setName(const QString& name);
        
        Item *child(int row);
        int childCount() const;
        virtual int columnCount() const;
        virtual QVariant data(int column) const;
        int row() const;
        Item *parentItem();
        const Item *parentItemConst() const;
        
        const Type type;
        
    protected:
        virtual void changed(int col);
        virtual void _removeChild(int index);
        
    protected:
        QString name;
        std::deque<Item*> childItems;
        Item* parent;
        
    protected slots:
        virtual void toOfflineState();
        
    };

}

#endif // MODELS_ITEM_H
