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

#ifndef SHADOWOVERLAY_H
#define SHADOWOVERLAY_H

#include <QWidget>
#include <QImage>
#include <QPainter>
#include <QColor>
#include <QPaintEvent>
#include <QResizeEvent>

#include <ui/utils/eb.h>

/**
 * @todo write docs
 */
class ShadowOverlay : public QWidget {
    
public:
    ShadowOverlay(unsigned int radius = 10, unsigned int thickness = 1, const QColor& color = Qt::black, QWidget* parent = nullptr);
    
    void setFrames(bool top, bool right, bool bottom, bool left);
    
protected:
    void updateImage();
    
    void paintEvent(QPaintEvent * event) override;
    void resizeEvent(QResizeEvent * event) override;
    
private:
    bool top;
    bool right;
    bool bottom;
    bool left;
    unsigned int thickness;
    unsigned int radius;
    QColor color;
    QImage shadow;
};

#endif // SHADOWOVERLAY_H
