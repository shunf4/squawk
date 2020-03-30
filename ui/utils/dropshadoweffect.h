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

#ifndef DROPSHADOWEFFECT_H
#define DROPSHADOWEFFECT_H

#include <QGraphicsEffect>
#include <QPainter>
#include <QPointF>
#include <QColor>

class PixmapFilter : public QObject
{
    Q_OBJECT
public:
    PixmapFilter(QObject *parent = nullptr);
    virtual ~PixmapFilter() = 0;
    
    virtual QRectF boundingRectFor(const QRectF &rect) const;
    virtual void draw(QPainter *painter, const QPointF &p, const QPixmap &src, const QRectF &srcRect = QRectF()) const = 0;
};

class PixmapDropShadowFilter : public PixmapFilter
{
    Q_OBJECT
    
public:
    PixmapDropShadowFilter(QObject *parent = nullptr);
    ~PixmapDropShadowFilter();
    
    void draw(QPainter *p, const QPointF &pos, const QPixmap &px, const QRectF &src = QRectF()) const override;
    
    qreal blurRadius() const;
    void setBlurRadius(qreal radius);
    
    QColor color() const;
    void setColor(const QColor &color);
    
    qreal thickness() const;
    void setThickness(qreal thickness);
    void setFrame(bool top, bool right, bool bottom, bool left);
    
protected:
    QColor mColor;
    qreal mRadius;
    qreal mThickness;
    bool top;
    bool right;
    bool bottom;
    bool left;
};

class DropShadowEffect : public QGraphicsEffect
{
    Q_OBJECT
public:
    qreal blurRadius() const;
    QColor color() const;
    void setFrame(bool top, bool right, bool bottom, bool left);
    void setThickness(qreal thickness);
    
signals:
    void blurRadiusChanged(qreal blurRadius);
    void colorChanged(const QColor &color);
    
public slots:
    void setBlurRadius(qreal blurRadius);
    void setColor(const QColor &color);
    
protected:
    void draw(QPainter * painter) override;
    
protected:
    PixmapDropShadowFilter filter;
    
};

#endif // DROPSHADOWEFFECT_H
