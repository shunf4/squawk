/*
 * Squawk messenger. 
 * Copyright (C) 2019 Yury Gubich <blue@macaw.me>
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

#ifndef BADGE_H
#define BADGE_H

#include <QFrame>
#include <QLabel>
#include <QHBoxLayout>
#include <QIcon>
#include <QPushButton>

/**
 * @todo write docs
 */
class Badge : public QFrame
{
    Q_OBJECT
public:
    Badge(const QString& id, const QString& text, const QIcon& icon, QWidget* parent = nullptr);
    ~Badge();
    
    const QString id;
    
signals:
    void close();
    
private:
    QLabel* image;
    QLabel* text;
    QPushButton* closeButton;
    QHBoxLayout* layout;
    
public:
    struct Comparator {
        bool operator()(const Badge& a, const Badge& b) const;
        bool operator()(const Badge* a, const Badge* b) const;
    };
};

#endif // BADGE_H
