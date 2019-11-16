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

#ifndef RESIZER_H
#define RESIZER_H

#include <QObject>
#include <QWidget>
#include <QEvent>
#include <QResizeEvent>

/**
 * @todo write docs
 */
class Resizer : public QObject {
    Q_OBJECT
public:
    Resizer(QWidget* parent = nullptr);
    
protected:
    bool eventFilter(QObject* obj, QEvent* event) override;
    
signals:
    void resized(const QSize& oldSize, const QSize& newSize);
};

#endif // RESIZER_H
