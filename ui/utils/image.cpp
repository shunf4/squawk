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

Image::Image(const QString& path, QWidget* parent):
    QLabel(parent),
    pixmap(path),
    aspectRatio(0)
{
    
    qreal height = pixmap.height();
    qreal width = pixmap.width();
    aspectRatio = width / height;
    setPixmap(pixmap);
    setScaledContents(true);
    setMinimumHeight(50 / aspectRatio);
    setMinimumWidth(50);
}

Image::~Image()
{

}

int Image::heightForWidth(int width) const
{
    int height = width / aspectRatio;
    //qDebug() << height << width << aspectRatio;
    return height;
}

bool Image::hasHeightForWidth() const
{
    return true;
}
