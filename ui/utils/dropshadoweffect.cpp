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

#include "dropshadoweffect.h"

PixmapFilter::PixmapFilter(QObject* parent):QObject(parent) {}
PixmapFilter::~PixmapFilter(){}
QRectF PixmapFilter::boundingRectFor(const QRectF &rect) const {return rect;}

PixmapDropShadowFilter::PixmapDropShadowFilter(QObject *parent):
    PixmapFilter(parent),
    mColor(63, 63, 63, 180),
    mRadius(1),
    mThickness(2),
    top(true),
    right(true),
    bottom(true),
    left(true){}

PixmapDropShadowFilter::~PixmapDropShadowFilter() {}
qreal PixmapDropShadowFilter::blurRadius() const {return mRadius;}
void PixmapDropShadowFilter::setBlurRadius(qreal radius) {mRadius = radius;}
QColor PixmapDropShadowFilter::color() const {return mColor;}
void PixmapDropShadowFilter::setColor(const QColor &color) {mColor = color;}
qreal PixmapDropShadowFilter::thickness() const {return mThickness;}
void PixmapDropShadowFilter::setThickness(qreal thickness) {mThickness = thickness;}
void PixmapDropShadowFilter::setFrame(bool ptop, bool pright, bool pbottom, bool pleft)
{
    top = ptop;
    right = pright;
    bottom = pbottom;
    left = pleft;
}

void DropShadowEffect::setThickness(qreal thickness)
{
    if (filter.thickness() == thickness)
        return;
    
    filter.setThickness(thickness);
    update();
}


void PixmapDropShadowFilter::draw(QPainter *p, const QPointF &pos, const QPixmap &px, const QRectF &src) const
{
    if (px.isNull())
        return;
    
    QImage tmp({px.width(), px.height() + int(mThickness)}, QImage::Format_ARGB32_Premultiplied);
    tmp.setDevicePixelRatio(px.devicePixelRatioF());
    tmp.fill(0);
    QPainter tmpPainter(&tmp);
    tmpPainter.setCompositionMode(QPainter::CompositionMode_Source);
    if (top) {
        QRectF shadow(0, 0, px.width(), mThickness);
        tmpPainter.fillRect(shadow, mColor);
    }
    if (right) {
        QRectF shadow(px.width() - mThickness, 0, mThickness, px.height());
        tmpPainter.fillRect(shadow, mColor);
    }
    if (bottom) {
        QRectF shadow(0, px.height() - mThickness, px.width(), mThickness * 2); //i have no idea why, but it leaves some unpainted stripe without some spare space 
        tmpPainter.fillRect(shadow, mColor);
    }
    if (left) {
        QRectF shadow(0, 0, mThickness, px.height());
        tmpPainter.fillRect(shadow, mColor);
    }
    
    Utils::exponentialblur(tmp, mRadius, false, 0);
    tmpPainter.end();
    
    // Draw the actual pixmap...
    p->drawPixmap(pos, px, src);
    
    // draw the blurred drop shadow...
    p->drawImage(pos, tmp);
}

qreal DropShadowEffect::blurRadius() const {return filter.blurRadius();}
void DropShadowEffect::setBlurRadius(qreal blurRadius)
{
    if (qFuzzyCompare(filter.blurRadius(), blurRadius))
        return;
    
    filter.setBlurRadius(blurRadius);
    updateBoundingRect();
    emit blurRadiusChanged(blurRadius);
}

void DropShadowEffect::setFrame(bool top, bool right, bool bottom, bool left)
{
    filter.setFrame(top, right, bottom, left);
    update();
}


QColor DropShadowEffect::color() const {return filter.color();}
void DropShadowEffect::setColor(const QColor &color)
{
    if (filter.color() == color)
        return;
    
    filter.setColor(color);
    update();
    emit colorChanged(color);
}

void DropShadowEffect::draw(QPainter* painter)
{
    if (filter.blurRadius() <= 0 && filter.thickness() == 0) {
        drawSource(painter);
        return;
    }
    
    PixmapPadMode mode = PadToEffectiveBoundingRect;
    
    // Draw pixmap in device coordinates to avoid pixmap scaling.
    QPoint offset;
    const QPixmap pixmap = sourcePixmap(Qt::DeviceCoordinates, &offset, mode);
    if (pixmap.isNull())
        return;
    
    QTransform restoreTransform = painter->worldTransform();
    painter->setWorldTransform(QTransform());
    filter.draw(painter, offset, pixmap);
    painter->setWorldTransform(restoreTransform);
}
