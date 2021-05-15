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

#ifndef PREVIEW_H
#define PREVIEW_H

#include <QWidget>
#include <QString>
#include <QPoint>
#include <QSize>
#include <QLabel>
#include <QIcon>
#include <QPixmap>
#include <QMovie>
#include <QFont>
#include <QFontMetrics>

#include <shared/global.h>

/**
 * @todo write docs
 */
class Preview {
public:
    Preview(const QString& pPath, const QSize& pMaxSize, const QPoint& pos, bool pRight, QWidget* parent);
    ~Preview();
    
    void actualize(const QString& newPath, const QSize& newSize, const QPoint& newPoint);
    void setPosition(const QPoint& newPoint);
    void setSize(const QSize& newSize);
    bool setPath(const QString& newPath);
    bool isFileReachable() const;
    QSize size() const;
    
    static QSize constrainAttachSize(QSize src, QSize bounds);
    static QSize calculateAttachSize(const QString& path, const QRect& bounds);
    static bool fontInitialized;
    static QFont font;
    static QFontMetrics metrics;
    
private:
    void initializeElements();
    void positionElements();
    void clean();
    void applyNewSize();
    void applyNewMaxSize();
    
private:
    Shared::Global::FileInfo info;
    QString path;
    QSize maxSize;
    QSize actualSize;
    QSize cachedLabelSize;
    QPoint position;
    QLabel* widget;
    QLabel* label;
    QWidget* parent;
    QMovie* movie;
    bool fileReachable;
    bool actualPreview;
    bool right;
};

#endif // PREVIEW_H
