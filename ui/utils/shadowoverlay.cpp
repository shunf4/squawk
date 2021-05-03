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

#include "shadowoverlay.h"

ShadowOverlay::ShadowOverlay(unsigned int r, unsigned int t, const QColor& c, QWidget* parent):
    QWidget(parent),
    top(false),
    right(false),
    bottom(false),
    left(false),
    thickness(t),
    radius(r),
    color(c),
    shadow(1, 1, QImage::Format_ARGB32_Premultiplied)
{
    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_TransparentForMouseEvents);
}

void ShadowOverlay::paintEvent(QPaintEvent* event)
{
    QWidget::paintEvent(event);
    
    QPainter painter(this);
    
    painter.drawImage(0, 0, shadow);
}

void ShadowOverlay::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    
    updateImage();
}

void ShadowOverlay::updateImage()
{
    int w = width();
    int h = height();
    shadow = QImage({w, h + int(thickness)}, QImage::Format_ARGB32_Premultiplied);
    shadow.fill(0);
    
    QPainter tmpPainter(&shadow);
    tmpPainter.setCompositionMode(QPainter::CompositionMode_Source);
    if (top) {
        QRectF shadow(0, 0, w, thickness);
        tmpPainter.fillRect(shadow, color);
    }
    if (right) {
        QRectF shadow(w - thickness, 0, thickness, h);
        tmpPainter.fillRect(shadow, color);
    }
    if (bottom) {
        QRectF shadow(0, h - thickness, w, thickness * 2); //i have no idea why, but it leaves some unpainted stripe without some spare space 
        tmpPainter.fillRect(shadow, color);
    }
    if (left) {
        QRectF shadow(0, 0, thickness, h);
        tmpPainter.fillRect(shadow, color);
    }
    
    Utils::exponentialblur(shadow, radius, false, 0);
    tmpPainter.end();
}

void ShadowOverlay::setFrames(bool t, bool r, bool b, bool l)
{
    top = t;
    right = r;
    bottom = b;
    left = l;
    
    updateImage();
    update();
}
