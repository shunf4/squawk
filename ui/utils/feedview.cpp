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

#include "feedview.h"

#include <QPaintEvent>
#include <QPainter>
#include <QDebug>

FeedView::FeedView(QWidget* parent):
    QAbstractItemView(parent),
    hints()
{

}

FeedView::~FeedView()
{
}

QModelIndex FeedView::indexAt(const QPoint& point) const
{
    return QModelIndex();
}

void FeedView::scrollTo(const QModelIndex& index, QAbstractItemView::ScrollHint hint)
{
}

QRect FeedView::visualRect(const QModelIndex& index) const
{
    if (!index.isValid() || index.row() >= hints.size()) {
        return QRect();
    } else {
        const Hint& hint = hints.at(index.row());
        const QWidget* vp = viewport();
        return QRect(0, vp->height() - hint.height - hint.offset, vp->width(), hint.height);
    }
}

int FeedView::horizontalOffset() const
{
    return 0;
}

bool FeedView::isIndexHidden(const QModelIndex& index) const
{
    return true;
}

QModelIndex FeedView::moveCursor(QAbstractItemView::CursorAction cursorAction, Qt::KeyboardModifiers modifiers)
{
    return QModelIndex();
}

void FeedView::setSelection(const QRect& rect, QItemSelectionModel::SelectionFlags command)
{
}

int FeedView::verticalOffset() const
{
    return 0;
}

QRegion FeedView::visualRegionForSelection(const QItemSelection& selection) const
{
    return QRegion();
}

void FeedView::rowsInserted(const QModelIndex& parent, int start, int end)
{
    scheduleDelayedItemsLayout();
    QAbstractItemView::rowsInserted(parent, start, end);
}

void FeedView::updateGeometries()
{
    qDebug() << "updateGeometries";
    QAbstractItemView::updateGeometries();
    const QAbstractItemModel* m = model();
    QStyleOptionViewItem option = viewOptions();
    uint32_t previousOffset = 0;
    
    hints.clear();
    for (int i = 0, size = m->rowCount(); i < size; ++i) {
        QModelIndex index = m->index(i, 0, QModelIndex());
        int height = itemDelegate(index)->sizeHint(option, index).height();
        hints.emplace_back(Hint({
            false,
            previousOffset,
            static_cast<uint32_t>(height)
        }));
        previousOffset += height;
    }
}

void FeedView::paintEvent(QPaintEvent* event)
{
    qDebug() << "paint";
    const QAbstractItemModel* m = model();
    QRect zone = event->rect().translated(horizontalOffset(), -verticalOffset());
    QPainter painter(viewport());
    QStyleOptionViewItem option = viewOptions();
    
    for (int i = 0, size = m->rowCount(); i < size; ++i) {
        QModelIndex index = m->index(i, 0, QModelIndex());
        option.rect = visualRect(index);
        itemDelegate(index)->paint(&painter, option, index);
    }
}
