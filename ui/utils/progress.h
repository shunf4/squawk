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

#ifndef PROGRESS_H
#define PROGRESS_H

#include <QWidget>
#include <QGraphicsScene>
#include <QIcon>
#include <QGraphicsView>
#include <QGraphicsPixmapItem>
#include <QVariantAnimation>
#include <QGridLayout>

/**
 * @todo write docs
 */
class Progress : public QWidget
{
    Q_OBJECT
public:
    Progress(quint16 p_size = 70, QWidget* parent = nullptr);
    ~Progress();
    
    void start();
    void stop();
    
private slots:
    void onValueChanged(const QVariant& value);
    
private:
    QGraphicsPixmapItem* pixmap;
    QGraphicsScene scene;
    QGraphicsView label;
    bool progress;
    QVariantAnimation animation;
    quint16 size;
};

#endif // PROGRESS_H
