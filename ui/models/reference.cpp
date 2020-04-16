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

using namespace Models;

Models::Reference::Reference(Models::Item* original, Models::Item* parent):
    Models::Item(reference, {}, parent),
    original(original)
{
    original->references.push_back(this);
    
    connect(original, &Item::childChanged, this, &Item::childChanged);
    connect(original, &Item::childIsAboutToBeInserted, this, &Item::childIsAboutToBeInserted);
    connect(original, &Item::childInserted, this, &Item::childInserted);
    connect(original, &Item::childIsAboutToBeRemoved, this, &Item::childIsAboutToBeRemoved);
    connect(original, &Item::childRemoved, this, &Item::childRemoved);
    connect(original, &Item::childIsAboutToBeMoved, this, &Item::childIsAboutToBeMoved);
    connect(original, &Item::childMoved, this, &Item::childMoved);
}

Models::Reference::~Reference()
{
    disconnect(original, &Item::childIsAboutToBeInserted, this, &Item::childIsAboutToBeInserted);
    disconnect(original, &Item::childInserted, this, &Item::childInserted);
    disconnect(original, &Item::childIsAboutToBeRemoved, this, &Item::childIsAboutToBeRemoved);
    disconnect(original, &Item::childRemoved, this, &Item::childRemoved);
    disconnect(original, &Item::childIsAboutToBeMoved, this, &Item::childIsAboutToBeMoved);
    disconnect(original, &Item::childMoved, this, &Item::childMoved);
    
    for (std::deque<Item*>::const_iterator itr = original->references.begin(), end = original->references.end(); itr != end; itr++) {
        if (*itr == this) {
            original->references.erase(itr);
            break;
        }
    }
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
