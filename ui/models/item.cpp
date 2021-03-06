/*
 * Squawk messenger. 
 * Copyright (C) 2019  Yury Gubich <blue@macaw.me>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "item.h"
#include "account.h"
#include "reference.h"
#include "contact.h"

#include <QDebug>

Models::Item::Item(Type p_type, const QMap<QString, QVariant> &p_data, Item *p_parent):
    QObject(),
    type(p_type),
    name(""),
    childItems(),
    parent(p_parent),
    references(),
    destroyingByParent(false),
    destroyingByOriginal(false)
{
    QMap<QString, QVariant>::const_iterator itr = p_data.find("name");
    if (itr != p_data.end()) {
        setName(itr.value().toString());
    }
}

Models::Item::Item(const Models::Item& other):
    QObject(),
    type(other.type),
    name(other.name),
    childItems(),
    parent(nullptr)
{
}

Models::Item::~Item()
{
    if (!destroyingByParent) {
        Item* parent = parentItem();
        if (parent != nullptr) {
            if (parent->type == reference) {
                parent->Item::removeChild(row());
            } else {
                parent->removeChild(row());
            }
        }
    }
    
    for (Reference* ref : references) {
        ref->destroyingByOriginal = true;
        delete ref;
    }
    
    for (Item* child : childItems) {
        child->destroyingByParent = true;
        delete child;
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
    _appendChild(child);
}

void Models::Item::_appendChild(Models::Item* child)
{
    bool moving = false;
    int newRow = 0;
    std::deque<Item*>::const_iterator before = childItems.begin();
    Type ct = child->type;
    if (ct == reference) {
        ct = static_cast<Reference*>(child)->dereference()->type;
    }
    while (before != childItems.end()) {
        Item* bfr = *before;
        Type bt = bfr->type;
        if (bt == reference) {
            bt = static_cast<Reference*>(bfr)->dereference()->type;
        }
        if (bt > ct) {
            break;
        } else if (bt == ct && bfr->getDisplayedName() > child->getDisplayedName()) {
            break;
        }
        newRow++;
        before++;
    }
    
    if (child->parent != nullptr) {
        int oldRow = child->row();
        moving = true;
        emit childIsAboutToBeMoved(child->parent, oldRow, oldRow, this, newRow);
        child->parent->_removeChild(oldRow);
    } else {
        emit childIsAboutToBeInserted(this, newRow, newRow);
    }
    childItems.insert(before, child);
    child->parent = this;
    
    QObject::connect(child, &Item::childChanged, this, &Item::childChanged);
    QObject::connect(child, &Item::childIsAboutToBeInserted, this, &Item::childIsAboutToBeInserted);
    QObject::connect(child, &Item::childInserted, this, &Item::childInserted);
    QObject::connect(child, &Item::childIsAboutToBeRemoved, this, &Item::childIsAboutToBeRemoved);
    QObject::connect(child, &Item::childRemoved, this, &Item::childRemoved);
    QObject::connect(child, &Item::childIsAboutToBeMoved, this, &Item::childIsAboutToBeMoved);
    QObject::connect(child, &Item::childMoved, this, &Item::childMoved);
    
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
    if (parent != nullptr) {
        std::deque<Item*>::const_iterator itr = parent->childItems.begin();
        std::deque<Item*>::const_iterator end = parent->childItems.end();
        
        for (int i = 0; itr != end; ++itr, ++i) {
            if (*itr == this) {
                return i;
            }
        }
    }
    
    return -1;       //TODO not sure how it helps
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
    return 2;
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
    
    QObject::disconnect(child, &Item::childChanged, this, &Item::childChanged);
    QObject::disconnect(child, &Item::childIsAboutToBeInserted, this, &Item::childIsAboutToBeInserted);
    QObject::disconnect(child, &Item::childInserted, this, &Item::childInserted);
    QObject::disconnect(child, &Item::childIsAboutToBeRemoved, this, &Item::childIsAboutToBeRemoved);
    QObject::disconnect(child, &Item::childRemoved, this, &Item::childRemoved);
    QObject::disconnect(child, &Item::childIsAboutToBeMoved, this, &Item::childIsAboutToBeMoved);
    QObject::disconnect(child, &Item::childMoved, this, &Item::childMoved);
    
    childItems.erase(childItems.begin() + index);
    child->parent = nullptr;
}


void Models::Item::changed(int col)
{
    
    emit childChanged(this, row(), col);
}

void Models::Item::toOfflineState()
{
    for (std::deque<Item*>::iterator itr = childItems.begin(), end = childItems.end(); itr != end; ++itr) {
        Item* it = *itr;
        it->toOfflineState();
    }
}

const Models::Account * Models::Item::getParentAccount() const
{
    const Item* p = this;
    
    while (p != nullptr && p->type != Item::account) {
        p = p->parentItemConst();
    }
    
    return static_cast<const Account*>(p);
}

QString Models::Item::getAccountJid() const
{
    const Account* acc = getParentAccount();
    if (acc == nullptr) {
        return "";
    }
    return acc->getLogin() + "@" + acc->getServer();
}

QString Models::Item::getAccountResource() const
{
    const Account* acc = getParentAccount();
    if (acc == nullptr) {
        return "";
    }
    return acc->getResource();
}

QString Models::Item::getAccountName() const
{
    const Account* acc = getParentAccount();
    if (acc == nullptr) {
        return "";
    }
    return acc->getName();
}

Shared::Availability Models::Item::getAccountAvailability() const
{
    const Account* acc = getParentAccount();
    if (acc == nullptr) {
        return Shared::Availability::offline;
    }
    return acc->getAvailability();
}

Shared::ConnectionState Models::Item::getAccountConnectionState() const
{
    const Account* acc = getParentAccount();
    if (acc == nullptr) {
        return Shared::ConnectionState::disconnected;
    }
    return acc->getState();
}

QString Models::Item::getAccountAvatarPath() const
{
    const Account* acc = getParentAccount();
    if (acc == nullptr) {
        return "";
    }
    return acc->getAvatarPath();
}

QString Models::Item::getDisplayedName() const
{
    return name;
}

void Models::Item::onChildChanged(Models::Item* item, int row, int col)
{
    Item* parent = item->parentItem();
    if (parent != nullptr && parent == this) {
        if (item->columnInvolvedInDisplay(col)) {
            int newRow = 0;
            std::deque<Item*>::const_iterator before = childItems.begin();
            
            Type ct = item->type;
            if (ct == reference) {
                ct = static_cast<Reference*>(item)->dereference()->type;
            }
            while (before != childItems.end()) {
                Item* bfr = *before;
                Type bt = bfr->type;
                if (bt == reference) {
                    bt = static_cast<Reference*>(bfr)->dereference()->type;
                }
                if (bt > ct) {
                    break;
                } else if (bt == ct && bfr->getDisplayedName() > item->getDisplayedName()) {
                    break;
                }
                newRow++;
                before++;
            }
            
            if (newRow != row || (before != childItems.end() && *before != item)) {
                emit childIsAboutToBeMoved(this, row, row, this, newRow);
                std::deque<Item*>::const_iterator old = childItems.begin();
                old += row;
                childItems.erase(old);
                childItems.insert(before, item);
                emit childMoved();
            }
        }
    }
    emit childChanged(item, row, col);
}

bool Models::Item::columnInvolvedInDisplay(int col)
{
    return col == 0;
}

void Models::Item::addReference(Models::Reference* ref)
{
    references.insert(ref);
}

void Models::Item::removeReference(Models::Reference* ref)
{
    std::set<Reference*>::const_iterator itr = references.find(ref);
    if (itr != references.end()) {
        references.erase(itr);
    }
}

int Models::Item::getContact(const QString& jid) const
{
    int index = -1;
    for (std::deque<Item*>::size_type i = 0; i < childItems.size(); ++i) {
        const Models::Item* item = childItems[i];
        const Contact* cnt = nullptr;
        if (item->type == Item::reference) {
            item = static_cast<const Reference*>(item)->dereferenceConst();
        }
        
        if (item->type == Item::contact) {
            cnt = static_cast<const Contact*>(item);
            if (cnt->getJid() == jid) {
                index = i;
                break;
            }
        }
    }
    return index;
}

std::set<Models::Reference *>::size_type Models::Item::referencesCount() const
{
    return references.size();
}
