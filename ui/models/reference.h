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

#ifndef MODELS_REFERENCE_H
#define MODELS_REFERENCE_H

#include "item.h"

namespace Models {

/**
 * @todo write docs
 */
class Reference : public Models::Item
{
    Q_OBJECT
public:
    Reference(Models::Item* original, Models::Item* parent);
    ~Reference();
    
    int columnCount() const override;
    QVariant data(int column) const override;
    QString getDisplayedName() const override;
    void appendChild(Models::Item * child) override;
    void removeChild(int index) override;
    Item* dereference();
    const Item* dereferenceConst() const;
    
protected slots:
    void toOfflineState() override;
    
private:
    Models::Item* original;
    
};

}

#endif // MODELS_REFERENCE_H
