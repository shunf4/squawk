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
#include <QScrollBar>
#include <QDebug>

constexpr int maxMessageHeight = 10000;
constexpr int approximateSingleMessageHeight = 20;

FeedView::FeedView(QWidget* parent):
    QAbstractItemView(parent),
    hints(),
    vo(0)
{
    horizontalScrollBar()->setRange(0, 0);
    verticalScrollBar()->setSingleStep(approximateSingleMessageHeight);
    setMouseTracking(true);
    setSelectionBehavior(SelectItems);
//     viewport()->setAttribute(Qt::WA_Hover, true);
}

FeedView::~FeedView()
{
}

QModelIndex FeedView::indexAt(const QPoint& point) const
{
    int32_t vh = viewport()->height();
    uint32_t y = vh - point.y() + vo;
    
    for (std::deque<Hint>::size_type i = 0; i < hints.size(); ++i) {
        if (hints[i].offset >= y) {
            return model()->index(i - 1, 0, rootIndex());
        }
    }
    
    return QModelIndex();
}

void FeedView::scrollTo(const QModelIndex& index, QAbstractItemView::ScrollHint hint)
{
}

QRect FeedView::visualRect(const QModelIndex& index) const
{
    if (!index.isValid() || index.row() >= hints.size()) {
        qDebug() << "visualRect for" << index.row();
        return QRect();
    } else {
        const Hint& hint = hints.at(index.row());
        const QWidget* vp = viewport();
        return QRect(0, vp->height() - hint.height - hint.offset + vo, vp->width(), hint.height);
    }
}

int FeedView::horizontalOffset() const
{
    return 0;
}

bool FeedView::isIndexHidden(const QModelIndex& index) const
{
    return false;
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
    return vo;
}

QRegion FeedView::visualRegionForSelection(const QItemSelection& selection) const
{
    return QRegion();
}

void FeedView::rowsInserted(const QModelIndex& parent, int start, int end)
{
    updateGeometries();
    QAbstractItemView::rowsInserted(parent, start, end);
}

void FeedView::updateGeometries()
{
    qDebug() << "updateGeometries";
    QScrollBar* bar = verticalScrollBar();
    
    QAbstractItemView::updateGeometries();
    
    const QStyle* st = style();
    const QAbstractItemModel* m = model();
    QRect layoutBounds = QRect(QPoint(), maximumViewportSize());
    QStyleOptionViewItem option = viewOptions();
    option.rect.setHeight(maxMessageHeight);
    option.rect.setWidth(layoutBounds.width());
    int frameAroundContents = 0;
    int verticalScrollBarExtent = st->pixelMetric(QStyle::PM_ScrollBarExtent, 0, bar);
    
    bool layedOut = false;
    if (verticalScrollBarExtent != 0 && verticalScrollBarPolicy() == Qt::ScrollBarAsNeeded && m->rowCount() * approximateSingleMessageHeight < layoutBounds.height()) {
        hints.clear();
        layedOut = tryToCalculateGeometriesWithNoScrollbars(option, m, layoutBounds.height());
    }
    
    if (layedOut) {
        bar->setRange(0, 0);
    } else {
        int verticalMargin = 0;
        if (st->styleHint(QStyle::SH_ScrollView_FrameOnlyAroundContents)) {
            frameAroundContents = st->pixelMetric(QStyle::PM_DefaultFrameWidth) * 2;
        }
        
        if (verticalScrollBarPolicy() == Qt::ScrollBarAsNeeded) {
            verticalMargin = verticalScrollBarExtent + frameAroundContents;
        }
        
        layoutBounds.adjust(0, 0, -verticalMargin, 0);
        
        option.features |= QStyleOptionViewItem::WrapText;
        option.rect.setWidth(layoutBounds.width());
        
        hints.clear();
        uint32_t previousOffset = 0;
        for (int i = 0, size = m->rowCount(); i < size; ++i) {
            QModelIndex index = m->index(i, 0, rootIndex());
            int height = itemDelegate(index)->sizeHint(option, index).height();
            hints.emplace_back(Hint({
                false,
                previousOffset,
                static_cast<uint32_t>(height)
            }));
            previousOffset += height;
        }
        
        bar->setRange(0, previousOffset - layoutBounds.height());
        bar->setPageStep(layoutBounds.height());
        bar->setValue(previousOffset - layoutBounds.height() - vo);
    }
}

bool FeedView::tryToCalculateGeometriesWithNoScrollbars(const QStyleOptionViewItem& option, const QAbstractItemModel* m, uint32_t totalHeight)
{
    uint32_t previousOffset = 0;
    bool success = true;
    for (int i = 0, size = m->rowCount(); i < size; ++i) {
        QModelIndex index = m->index(i, 0, rootIndex());
        int height = itemDelegate(index)->sizeHint(option, index).height();
        
        if (previousOffset + height > totalHeight) {
            success = false;
            break;
        }
        hints.emplace_back(Hint({
            false,
            previousOffset,
            static_cast<uint32_t>(height)
        }));
        previousOffset += height;
    }
    
    return success;
}


void FeedView::paintEvent(QPaintEvent* event)
{
    //qDebug() << "paint" << event->rect();
    const QAbstractItemModel* m = model();
    QWidget* vp = viewport();
    QRect zone = event->rect().translated(0, -vo);
    uint32_t vph = vp->height(); 
    int32_t y1 = zone.y();
    int32_t y2 = y1 + zone.height();
    
    bool inZone = false;
    std::deque<QModelIndex> toRener;
    for (std::deque<Hint>::size_type i = 0; i < hints.size(); ++i) {
        const Hint& hint = hints[i];
        int32_t relativeY1 = vph - hint.offset - hint.height;
        if (!inZone) {
            if (y2 > relativeY1) {
                inZone = true;
            }
        }
        if (inZone) {
            toRener.emplace_back(m->index(i, 0, rootIndex()));
        }
        if (y1 > relativeY1) {
            break;
        }
    }
    
    QPainter painter(vp);
    QStyleOptionViewItem option = viewOptions();
    option.features = QStyleOptionViewItem::WrapText;
    QPoint cursor = vp->mapFromGlobal(QCursor::pos());
    
    for (const QModelIndex& index : toRener) {
        option.rect = visualRect(index);
        option.state.setFlag(QStyle::State_MouseOver, option.rect.contains(cursor));
        itemDelegate(index)->paint(&painter, option, index);
    }
}

void FeedView::verticalScrollbarValueChanged(int value)
{
    vo = verticalScrollBar()->maximum() - value;
    
    QAbstractItemView::verticalScrollbarValueChanged(vo);
}

void FeedView::mouseMoveEvent(QMouseEvent* event)
{
    if (!isVisible()) {
        return;
    }
    
    QAbstractItemView::mouseMoveEvent(event);
}


QFont FeedView::getFont() const
{
    return viewOptions().font;
}
