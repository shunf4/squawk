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

#include "messagedelegate.h"
#include "ui/models/messagefeed.h"

constexpr int maxMessageHeight = 10000;
constexpr int approximateSingleMessageHeight = 20;
constexpr int progressSize = 70;

const std::set<int> FeedView::geometryChangingRoles = {
    Models::MessageFeed::Attach,
    Models::MessageFeed::Text,
    Models::MessageFeed::Id,
    Models::MessageFeed::Error
    
};

FeedView::FeedView(QWidget* parent):
    QAbstractItemView(parent),
    hints(),
    vo(0),
    specialDelegate(false),
    specialModel(false),
    clearWidgetsMode(false),
    modelState(Models::MessageFeed::complete),
    progress()
{
    horizontalScrollBar()->setRange(0, 0);
    verticalScrollBar()->setSingleStep(approximateSingleMessageHeight);
    setMouseTracking(true);
    setSelectionBehavior(SelectItems);
//     viewport()->setAttribute(Qt::WA_Hover, true);
    
    progress.setParent(viewport());
    progress.resize(progressSize, progressSize);
}

FeedView::~FeedView()
{
}

QModelIndex FeedView::indexAt(const QPoint& point) const
{
    int32_t vh = viewport()->height();
    uint32_t y = vh - point.y() + vo;
    
    for (std::deque<Hint>::size_type i = 0; i < hints.size(); ++i) {
        if (hints[i].offset + hints[i].height >= y) {
            return model()->index(i, 0, rootIndex());
        }
    }
    
    return QModelIndex();
}

void FeedView::scrollTo(const QModelIndex& index, QAbstractItemView::ScrollHint hint)
{
}

QRect FeedView::visualRect(const QModelIndex& index) const
{
    unsigned int row = index.row();
    if (!index.isValid() || row >= hints.size()) {
        qDebug() << "visualRect for" << row;
        return QRect();
    } else {
        const Hint& hint = hints.at(row);
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
    QAbstractItemView::rowsInserted(parent, start, end);
    
    scheduleDelayedItemsLayout();
}

void FeedView::dataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles)
{
    if (specialDelegate) {
        for (int role : roles) {
            if (geometryChangingRoles.count(role) != 0) {
                scheduleDelayedItemsLayout();                         //to recalculate layout only if there are some geometry changing modifications
                break;
            }
        }
    }
    QAbstractItemView::dataChanged(topLeft, bottomRight, roles);
}

void FeedView::updateGeometries()
{
    qDebug() << "updateGeometries";
    QScrollBar* bar = verticalScrollBar();
    
    const QStyle* st = style();
    const QAbstractItemModel* m = model();
    QSize layoutBounds = maximumViewportSize();
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
        vo = 0;
    } else {
        int verticalMargin = 0;
        if (st->styleHint(QStyle::SH_ScrollView_FrameOnlyAroundContents)) {
            frameAroundContents = st->pixelMetric(QStyle::PM_DefaultFrameWidth) * 2;
        }
        
        if (verticalScrollBarPolicy() == Qt::ScrollBarAsNeeded) {
            verticalMargin = verticalScrollBarExtent + frameAroundContents;
        }
        
        layoutBounds.rwidth() -= verticalMargin;
        
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
        
        int totalHeight = previousOffset - layoutBounds.height();
        if (modelState != Models::MessageFeed::complete) {
            totalHeight += progressSize;
        }
        vo = qMax(qMin(vo, totalHeight), 0);
        bar->setRange(0, totalHeight);
        bar->setPageStep(layoutBounds.height());
        bar->setValue(totalHeight - vo);
    }
    
    positionProgress();
    
    if (specialDelegate) {
        clearWidgetsMode = true;
    }
    
    
    QAbstractItemView::updateGeometries();
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
    
    if (specialDelegate) {
        MessageDelegate* del = static_cast<MessageDelegate*>(itemDelegate());
        if (clearWidgetsMode) {
            del->beginClearWidgets();
        }
    }
    
    for (const QModelIndex& index : toRener) {
        option.rect = visualRect(index);
        option.state.setFlag(QStyle::State_MouseOver, option.rect.contains(cursor));
        itemDelegate(index)->paint(&painter, option, index);
    }
    
    if (clearWidgetsMode && specialDelegate) {
        MessageDelegate* del = static_cast<MessageDelegate*>(itemDelegate());
        del->endClearWidgets();
        clearWidgetsMode = false;
    }
}

void FeedView::verticalScrollbarValueChanged(int value)
{
    vo = verticalScrollBar()->maximum() - value;
    
    positionProgress();
    
    if (specialDelegate) {
        clearWidgetsMode = true;
    }
    
    QAbstractItemView::verticalScrollbarValueChanged(vo);
}

void FeedView::mouseMoveEvent(QMouseEvent* event)
{
    if (!isVisible()) {
        return;
    }
    
    QAbstractItemView::mouseMoveEvent(event);
}

void FeedView::resizeEvent(QResizeEvent* event)
{
    QAbstractItemView::resizeEvent(event);
    
    positionProgress();
}

void FeedView::positionProgress()
{
    QSize layoutBounds = maximumViewportSize();
    int progressPosition = layoutBounds.height() - progressSize;
    std::deque<Hint>::size_type size = hints.size();
    if (size > 0) {
        const Hint& hint = hints[size - 1];
        progressPosition -= hint.offset + hint.height;
    }
    progressPosition += vo;
    
    progress.move((width() - progressSize) / 2, progressPosition);
}

QFont FeedView::getFont() const
{
    return viewOptions().font;
}

void FeedView::setItemDelegate(QAbstractItemDelegate* delegate)
{
    if (specialDelegate) {
        MessageDelegate* del = static_cast<MessageDelegate*>(itemDelegate());
        disconnect(del, &MessageDelegate::buttonPushed, this, &FeedView::onMessageButtonPushed);
        disconnect(del, &MessageDelegate::invalidPath, this, &FeedView::onMessageInvalidPath);
    }
    
    QAbstractItemView::setItemDelegate(delegate);
    
    MessageDelegate* del = dynamic_cast<MessageDelegate*>(delegate);
    if (del) {
        specialDelegate = true;
        connect(del, &MessageDelegate::buttonPushed, this, &FeedView::onMessageButtonPushed);
        connect(del, &MessageDelegate::invalidPath, this, &FeedView::onMessageInvalidPath);
    } else {
        specialDelegate = false;
    }
}

void FeedView::setModel(QAbstractItemModel* p_model)
{
    if (specialModel) {
        Models::MessageFeed* feed = static_cast<Models::MessageFeed*>(model());
        disconnect(feed, &Models::MessageFeed::syncStateChange, this, &FeedView::onModelSyncStateChange);
    }
    
    QAbstractItemView::setModel(p_model);
    
    Models::MessageFeed* feed = dynamic_cast<Models::MessageFeed*>(p_model);
    if (feed) {
        onModelSyncStateChange(feed->getSyncState());
        specialModel = true;
        connect(feed, &Models::MessageFeed::syncStateChange, this, &FeedView::onModelSyncStateChange);
    } else {
        onModelSyncStateChange(Models::MessageFeed::complete);
        specialModel = false;
    }
}

void FeedView::onMessageButtonPushed(const QString& messageId, bool download)
{
    if (specialModel) {
        Models::MessageFeed* feed = static_cast<Models::MessageFeed*>(model());
        
        if (download) {
            feed->downloadAttachment(messageId);
        } else {
            feed->uploadAttachment(messageId);
        }
    }
}

void FeedView::onMessageInvalidPath(const QString& messageId)
{
    if (specialModel) {
        Models::MessageFeed* feed = static_cast<Models::MessageFeed*>(model());
        feed->reportLocalPathInvalid(messageId);
    }
}

void FeedView::onModelSyncStateChange(Models::MessageFeed::SyncState state)
{
    bool needToUpdateGeometry = false;
    if (modelState != state) {
        if (state == Models::MessageFeed::complete || modelState == Models::MessageFeed::complete) {
            needToUpdateGeometry = true;
        }
        modelState = state;
        
        if (state == Models::MessageFeed::syncing) {
            progress.show();
            progress.start();
        } else {
            progress.stop();
            progress.hide();
        }
    }
    
    if (needToUpdateGeometry) {
        scheduleDelayedItemsLayout();
    }
}
