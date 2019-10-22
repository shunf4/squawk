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

#include <QDebug>
#include "image.h"

Image::Image(const QString& path, quint16 p_minWidth, QWidget* parent):
    QLabel(parent),
    pixmap(path),
    aspectRatio(0),
    minWidth(p_minWidth)
{
    setScaledContents(true);
    recalculateAspectRatio();
}

Image::Image(const QString& path, quint16 width, quint16 height, quint16 p_minWidth, QWidget* parent):
    QLabel(parent),
    pixmap(QIcon(path).pixmap(QSize(width, height))),
    aspectRatio(0),
    minWidth(p_minWidth)
{
    setScaledContents(true);
    recalculateAspectRatio();
}

Image::~Image()
{

}

int Image::heightForWidth(int width) const
{
    int height = width / aspectRatio;
    return height;
}

int Image::widthForHeight(int height) const
{
    return height * aspectRatio;
}

bool Image::hasHeightForWidth() const
{
    return true;
}

void Image::recalculateAspectRatio()
{
    qreal height = pixmap.height();
    qreal width = pixmap.width();
    aspectRatio = width / height;
    setPixmap(pixmap);
    setMinimumHeight(minWidth / aspectRatio);
    setMinimumWidth(minWidth);
}

void Image::setMinWidth(quint16 p_minWidth)
{
    if (minWidth != p_minWidth) {
        minWidth = p_minWidth;
        recalculateAspectRatio();
    }
}

void Image::setPath(const QString& path)
{
    pixmap = QPixmap(path);
    recalculateAspectRatio();
}

void Image::setPath(const QString& path, quint16 width, quint16 height)
{
    pixmap = QPixmap(QIcon(path).pixmap(QSize(width, height)));
    recalculateAspectRatio();
}
