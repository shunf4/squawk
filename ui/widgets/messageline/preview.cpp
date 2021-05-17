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

#include "preview.h"


constexpr int margin = 6;
constexpr int maxAttachmentHeight = 500;

QFont Preview::font;
QFontMetrics Preview::metrics(Preview::font);

Preview::Preview(const QString& pPath, const QSize& pMaxSize, const QPoint& pos, bool pRight, QWidget* pParent):
    info(Shared::Global::getFileInfo(pPath)),
    path(pPath),
    maxSize(pMaxSize),
    actualSize(constrainAttachSize(info.size, maxSize)),
    cachedLabelSize(0, 0),
    position(pos),
    widget(0),
    label(0),
    parent(pParent),
    movie(0),
    fileReachable(true),
    actualPreview(false),
    right(pRight)
{
    
    initializeElements();
    if (fileReachable) {
        positionElements();
    }
}

void Preview::initializeFont(const QFont& newFont)
{
    font = newFont;
    font.setBold(true);
    metrics = QFontMetrics(font);
}

Preview::~Preview()
{
    clean();
}

void Preview::clean()
{
    if (fileReachable) {
        if (info.preview == Shared::Global::FileInfo::Preview::animation) {
            delete movie;
        }
        delete widget;
        if (!actualPreview) {
            delete label;
        } else {
            actualPreview = false;
        }
    } else {
        fileReachable = true;
    }
}

void Preview::actualize(const QString& newPath, const QSize& newSize, const QPoint& newPoint)
{
    bool positionChanged = false;
    bool sizeChanged = false;
    bool maxSizeChanged = false;
    
    if (maxSize != newSize) {
        maxSize = newSize;
        maxSizeChanged = true;
        QSize ns = constrainAttachSize(info.size, maxSize);
        if (actualSize != ns) {
            sizeChanged = true;
            actualSize = ns;
        }
    }
    if (position != newPoint) {
        position = newPoint;
        positionChanged = true;
    }
    
    if (!setPath(newPath) && fileReachable) {
        if (sizeChanged) {
            applyNewSize();
            if (maxSizeChanged && !actualPreview) {
                applyNewMaxSize();
            }
        } else if (maxSizeChanged) {
            applyNewMaxSize();
            if (right) {
                positionChanged = true;
            }
        }
        if (positionChanged || !actualPreview) {
            positionElements();
        }
    }
}

void Preview::setSize(const QSize& newSize)
{
    bool sizeChanged = false;
    bool maxSizeChanged = false;
    
    if (maxSize != newSize) {
        maxSize = newSize;
        maxSizeChanged = true;
        QSize ns = constrainAttachSize(info.size, maxSize);
        if (actualSize != ns) {
            sizeChanged = true;
            actualSize = ns;
        }
    }
    
    if (fileReachable) {
        if (sizeChanged) {
            applyNewSize();
        }
        if (maxSizeChanged || !actualPreview) {
            applyNewMaxSize();
            if (right) {
                positionElements();
            }
        }
    }
}

void Preview::applyNewSize()
{
    switch (info.preview) {
        case Shared::Global::FileInfo::Preview::picture: {
            QImageReader img(path);
            if (!img.canRead()) {
                delete widget;
                fileReachable = false;
            } else {
                img.setScaledSize(actualSize);
                widget->resize(actualSize);
                widget->setPixmap(QPixmap::fromImage(img.read()));
            }
        }
            break;
        case Shared::Global::FileInfo::Preview::animation:{
            movie->setScaledSize(actualSize);
            widget->resize(actualSize);
        }
            break;
        default: {
            QIcon icon = QIcon::fromTheme(info.mime.iconName());
            widget->setPixmap(icon.pixmap(actualSize));
            widget->resize(actualSize);
        }
    }
}

void Preview::applyNewMaxSize()
{
    switch (info.preview) {
        case Shared::Global::FileInfo::Preview::picture: 
        case Shared::Global::FileInfo::Preview::animation: 
            break;
        default: {
            int labelWidth = maxSize.width() - actualSize.width() - margin;
            QString elidedName = metrics.elidedText(info.name, Qt::ElideMiddle, labelWidth);
            cachedLabelSize = metrics.boundingRect(elidedName).size();
            label->setText(elidedName);
            label->resize(cachedLabelSize);
        }
    }
}


QSize Preview::size() const
{
    if (actualPreview) {
        return actualSize;
    } else {
        return QSize(actualSize.width() + margin + cachedLabelSize.width(), actualSize.height());
    }
}

bool Preview::isFileReachable() const
{
    return fileReachable;
}

void Preview::setPosition(const QPoint& newPoint)
{
    if (position != newPoint) {
        position = newPoint;
        if (fileReachable) {
            positionElements();
        }
    }
}

bool Preview::setPath(const QString& newPath)
{
    if (path != newPath) {
        path = newPath;
        info = Shared::Global::getFileInfo(path);
        actualSize = constrainAttachSize(info.size, maxSize);
        clean();
        initializeElements();
        if (fileReachable) {
            positionElements();
        }
        return true;
    } else {
        return false;
    }
}

void Preview::initializeElements()
{
    switch (info.preview) {
        case Shared::Global::FileInfo::Preview::picture: {
            QImageReader img(path);
            if (!img.canRead()) {
                fileReachable = false;
            } else {
                actualPreview = true;
                img.setScaledSize(actualSize);
                widget = new QLabel(parent);
                widget->setPixmap(QPixmap::fromImage(img.read()));
                widget->show();
            }
        }
            break;
        case Shared::Global::FileInfo::Preview::animation:{
            movie = new QMovie(path);
            QObject::connect(movie, &QMovie::error, 
                std::bind(&Preview::handleQMovieError, this, std::placeholders::_1)
            );
            if (!movie->isValid()) {
                fileReachable = false;
                delete movie;
            } else {
                actualPreview = true;
                movie->setScaledSize(actualSize);
                widget = new QLabel(parent);
                widget->setMovie(movie);
                movie->start();
                widget->show();
            }
        }
            break;
        default: {
            QIcon icon = QIcon::fromTheme(info.mime.iconName());
            widget = new QLabel(parent);
            widget->setPixmap(icon.pixmap(actualSize));
            widget->show();
            
            label = new QLabel(parent);
            label->setFont(font);
            int labelWidth = maxSize.width() - actualSize.width() - margin;
            QString elidedName = metrics.elidedText(info.name, Qt::ElideMiddle, labelWidth);
            cachedLabelSize = metrics.boundingRect(elidedName).size();
            label->setText(elidedName);
            label->show();
        }
    }
}

void Preview::positionElements()
{
    int start = position.x();
    if (right) {
        start += maxSize.width() - size().width();
    }
    widget->move(start, position.y());
    if (!actualPreview) {
        int x = start + actualSize.width() + margin;
        int y = position.y() + (actualSize.height() - cachedLabelSize.height()) / 2;
        label->move(x, y);
    }
}

QSize Preview::calculateAttachSize(const QString& path, const QRect& bounds)
{
    Shared::Global::FileInfo info = Shared::Global::getFileInfo(path);
    
    return constrainAttachSize(info.size, bounds.size());
}

QSize Preview::constrainAttachSize(QSize src, QSize bounds)
{
    if (bounds.height() > maxAttachmentHeight) {
        bounds.setHeight(maxAttachmentHeight);
    }
    
    if (src.width() > bounds.width() || src.height() > bounds.height()) {
        src.scale(bounds, Qt::KeepAspectRatio);
    }
    
    return src;
}

void Preview::handleQMovieError(QImageReader::ImageReaderError error)
{
    if (error == QImageReader::FileNotFoundError) {
        fileReachable = false;
        movie->deleteLater();
        widget->deleteLater();
    }
}
