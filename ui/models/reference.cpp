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

#include "reference.h"
#include <QDebug>

using namespace Models;

Models::Reference::Reference(Models::Item* original, Models::Item* parent):
    Models::Item(reference, {}, parent),
    original(original),
    ax(-1),
    bx(-1),
    cx(-1),
    c(false)
{
    connect(original, &Item::childChanged, this, &Reference::onChildChanged, Qt::QueuedConnection);
    connect(original, &Item::childIsAboutToBeInserted, this, &Reference::onChildIsAboutToBeInserted);
    connect(original, &Item::childInserted, this, &Reference::onChildInserted);
    connect(original, &Item::childIsAboutToBeRemoved, this, &Reference::onChildIsAboutToBeRemoved);
    connect(original, &Item::childRemoved, this, &Reference::onChildRemoved);
    connect(original, &Item::childIsAboutToBeMoved, this, &Reference::onChildIsAboutToBeMoved);
    connect(original, &Item::childMoved, this, &Reference::onChildMoved);
    
    original->addReference(this);
    
    for (int i = 0; i < original->childCount(); i++) {
        Reference* ref = new Reference(original->child(i));
        Item::appendChild(ref);
    }
}

Models::Reference::~Reference()
{
    if (!destroyingByOriginal) {
        original->removeReference(this);
    }
    
    disconnect(original, &Item::childChanged, this, &Reference::onChildChanged);
    disconnect(original, &Item::childIsAboutToBeInserted, this, &Reference::onChildIsAboutToBeInserted);
    disconnect(original, &Item::childInserted, this, &Reference::onChildInserted);
    disconnect(original, &Item::childIsAboutToBeRemoved, this, &Reference::onChildIsAboutToBeRemoved);
    disconnect(original, &Item::childRemoved, this, &Reference::onChildRemoved);
    disconnect(original, &Item::childIsAboutToBeMoved, this, &Reference::onChildIsAboutToBeMoved);
    disconnect(original, &Item::childMoved, this, &Reference::onChildMoved);
}

int Models::Reference::columnCount() const
{
    return original->columnCount();
}

QVariant Models::Reference::data(int column) const
{
    return original->data(column);
}

QString Models::Reference::getDisplayedName() const
{
    return original->getDisplayedName();
}

Models::Item * Models::Reference::dereference()
{
    return original;
}

const Models::Item * Models::Reference::dereferenceConst() const
{
    return original;
}

void Models::Reference::appendChild(Models::Item* child)
{
    original->appendChild(child);
}

void Models::Reference::removeChild(int index)
{
    original->removeChild(index);
}

void Models::Reference::toOfflineState()
{
    original->toOfflineState();
}

void Models::Reference::onChildChanged(Models::Item* item, int row, int col)
{
    if (item == original) {
        emit childChanged(this, row, col);
    }
}

void Models::Reference::onChildIsAboutToBeInserted(Models::Item* parent, int first, int last)
{
    if (parent == original) {
        ax = first;
        bx = last;
    }
}

void Models::Reference::onChildInserted()
{
    if (ax != -1 && bx != -1) {
        for (int i = ax; i <= bx; ++i) {
            Reference* ref = new Reference(original->child(i));
            Item::appendChild(ref);
        }
        ax = -1;
        bx = -1;
    }
}

void Models::Reference::onChildIsAboutToBeRemoved(Models::Item* parent, int first, int last)
{
    if (parent == original) {
        emit childIsAboutToBeRemoved(this, first, last);
        for (int i = first; i <= last; ++i) {
            Reference* ref = static_cast<Reference*>(childItems[first]);
            Item::_removeChild(first);
            delete ref;
        }
        childRemoved();
    }
}

void Models::Reference::onChildRemoved()
{
}

void Models::Reference::onChildIsAboutToBeMoved(Models::Item* source, int first, int last, Models::Item* destination, int newIndex)
{
    if (destination == original) {
        if (source != original) {
            c = true;
            ax = first;
            bx = last;
            cx = newIndex;
            emit childIsAboutToBeMoved(source, first, last, destination, newIndex);
        } else {
            ax = newIndex;
            bx = newIndex + last - first + 1;
        }
    }
}

void Models::Reference::onChildMoved()
{
    if (c) {
        std::deque<Item*>::const_iterator beg = childItems.begin() + ax;
        std::deque<Item*>::const_iterator end = childItems.begin() + bx + 1;
        std::deque<Item*>::const_iterator tgt = childItems.begin() + cx;
        std::deque<Item*> temp;
        temp.insert(temp.end(), beg, end);
        childItems.erase(beg, end);
        childItems.insert(tgt, temp.begin(), temp.end());
        emit childMoved();
    } else {
        onChildInserted();
    }
}
