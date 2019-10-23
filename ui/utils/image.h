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

#ifndef IMAGE_H
#define IMAGE_H

#include <QLabel>
#include <QPixmap>

/**
 * @todo write docs
 */
class Image : public QLabel
{
public:
    Image(const QString& path, quint16 minWidth = 50, QWidget* parent = nullptr);

    ~Image();

    int heightForWidth(int width) const override;
    int widthForHeight(int height) const;
    bool hasHeightForWidth() const override;
    void setPath(const QString& path);
    void setMinWidth(quint16 minWidth);
    
private:
    QPixmap pixmap;
    qreal aspectRatio;
    quint16 minWidth;
    
private:
    void recalculateAspectRatio();
};

#endif // IMAGE_H
