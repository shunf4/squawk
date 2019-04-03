#include "item.h"

using namespace Models;

Models::Item::Item(Type p_type, const QMap<QString, QVariant> &p_data, Item *p_parent):
    type(p_type),
    name(p_data.value("name").toString()),
    childItems(),
    parent(p_parent)
{}

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
    name = p_name;
}

void Models::Item::appendChild(Models::Item* child)
{
    childItems.push_back(child);
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
