#include "item.h"

Models::Item::Item(Type p_type, const QMap<QString, QVariant> &p_data, Item *p_parent):
    QObject(),
    type(p_type),
    name(""),
    childItems(),
    parent(p_parent)
{
    QMap<QString, QVariant>::const_iterator itr = p_data.find("name");
    if (itr != p_data.end()) {
        setName(itr.value().toString());
    }
}

Models::Item::~Item()
{
    std::deque<Item*>::const_iterator itr = childItems.begin();
    std::deque<Item*>::const_iterator end = childItems.end();
    
    for (;itr != end; ++itr) {
        delete (*itr);
    }
}

void Models::Item::setName(const QString& p_name)
{
    if (name != p_name) {
        name = p_name;
        changed(0);
    }
}

void Models::Item::appendChild(Models::Item* child)
{
    bool moving = false;
    int oldRow = child->row();
    int newRow = this->childCount();
    if (child->parent != 0) {
        moving = true;
        emit childIsAboutToBeMoved(child->parent, oldRow, oldRow, this, newRow);
        child->parent->_removeChild(oldRow);
    } else {
        emit childIsAboutToBeInserted(this, newRow, newRow);
    }
    childItems.push_back(child);
    child->parent = this;
    
    QObject::connect(child, SIGNAL(childChanged(Models::Item*, int, int)), this, SIGNAL(childChanged(Models::Item*, int, int)));
    QObject::connect(child, SIGNAL(childIsAboutToBeInserted(Item*, int, int)), this, SIGNAL(childIsAboutToBeInserted(Item*, int, int)));
    QObject::connect(child, SIGNAL(childInserted()), this, SIGNAL(childInserted()));
    QObject::connect(child, SIGNAL(childIsAboutToBeRemoved(Item*, int, int)), this, SIGNAL(childIsAboutToBeRemoved(Item*, int, int)));
    QObject::connect(child, SIGNAL(childRemoved()), this, SIGNAL(childRemoved()));
    QObject::connect(child, SIGNAL(childIsAboutToBeMoved(Item*, int, int, Item*, int)), this, SIGNAL(childIsAboutToBeMoved(Item*, int, int, Item*, int)));
    QObject::connect(child, SIGNAL(childMoved()), this, SIGNAL(childMoved()));
    
    if (moving) {
        emit childMoved();
    } else {
        emit childInserted();
    }
}

Models::Item * Models::Item::child(int row)
{
    return childItems[row];
}

int Models::Item::childCount() const
{
    return childItems.size();
}

int Models::Item::row() const
{
    if (parent != 0) {
        std::deque<Item*>::const_iterator itr = parent->childItems.begin();
        std::deque<Item*>::const_iterator end = parent->childItems.end();
        
        for (int i = 0; itr != end; ++itr, ++i) {
            if (*itr == this) {
                return i;
            }
        }
    }
    
    return 0;       //TODO not sure how it helps, i copy-pasted it from the example
}

Models::Item * Models::Item::parentItem()
{
    return parent;
}

const Models::Item * Models::Item::parentItemConst() const
{
    return parent;
}

int Models::Item::columnCount() const
{
    return 1;
}

QString Models::Item::getName() const
{
    return name;
}

QVariant Models::Item::data(int column) const
{
    if (column != 0) {
        return QVariant();
    }
    return name;
}

void Models::Item::removeChild(int index)
{
    emit childIsAboutToBeRemoved(this, index, index);
    _removeChild(index);
    emit childRemoved();
}

void Models::Item::_removeChild(int index)
{
    Item* child = childItems[index];
    
    QObject::disconnect(child, SIGNAL(childChanged(Models::Item*, int, int)), this, SIGNAL(childChanged(Models::Item*, int, int)));
    QObject::disconnect(child, SIGNAL(childIsAboutToBeInserted(Item*, int, int)), this, SIGNAL(childIsAboutToBeInserted(Item*, int, int)));
    QObject::disconnect(child, SIGNAL(childInserted()), this, SIGNAL(childInserted()));
    QObject::disconnect(child, SIGNAL(childIsAboutToBeRemoved(Item*, int, int)), this, SIGNAL(childIsAboutToBeRemoved(Item*, int, int)));
    QObject::disconnect(child, SIGNAL(childRemoved()), this, SIGNAL(childRemoved()));
    QObject::disconnect(child, SIGNAL(childIsAboutToBeMoved(Item*, int, int, Item*, int)), this, SIGNAL(childIsAboutToBeMoved(Item*, int, int, Item*, int)));
    QObject::disconnect(child, SIGNAL(childMoved()), this, SIGNAL(childMoved()));
    
    childItems.erase(childItems.begin() + index);
    child->parent = 0;
}


void Models::Item::changed(int col)
{
    if (parent != 0) {
        emit childChanged(this, row(), col);
    }
}

void Models::Item::toOfflineState()
{
    for (std::deque<Item*>::iterator itr = childItems.begin(), end = childItems.end(); itr != end; ++itr) {
        Item* it = *itr;
        it->toOfflineState();
    }
}
