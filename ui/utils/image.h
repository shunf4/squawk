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
    /**
     * Default constructor
     */
    Image(const QString& path, QWidget* parent = nullptr);

    /**
     * Destructor
     */
    ~Image();

    /**
     * @todo write docs
     *
     * @param  TODO
     * @return TODO
     */
    int heightForWidth(int width) const override;

    /**
     * @todo write docs
     *
     * @return TODO
     */
    virtual bool hasHeightForWidth() const;
    
private:
    QPixmap pixmap;
    qreal aspectRatio;
};

#endif // IMAGE_H
