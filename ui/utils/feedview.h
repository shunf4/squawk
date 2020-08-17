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

#ifndef FEEDVIEW_H
#define FEEDVIEW_H

#include <QAbstractItemView>

#include <deque>

#include <ui/models/messagefeed.h>

/**
 * @todo write docs
 */
class FeedView : public QAbstractItemView
{
    Q_OBJECT
public:
    FeedView(QWidget* parent = nullptr);
    ~FeedView();
    
    QModelIndex indexAt(const QPoint & point) const override;
    void scrollTo(const QModelIndex & index, QAbstractItemView::ScrollHint hint) override;
    QRect visualRect(const QModelIndex & index) const override;
    bool isIndexHidden(const QModelIndex & index) const override;
    QModelIndex moveCursor(QAbstractItemView::CursorAction cursorAction, Qt::KeyboardModifiers modifiers) override;
    void setSelection(const QRect & rect, QItemSelectionModel::SelectionFlags command) override;
    QRegion visualRegionForSelection(const QItemSelection & selection) const override;
    
protected slots:
    void rowsInserted(const QModelIndex & parent, int start, int end) override;
    void verticalScrollbarValueChanged(int value) override;
    
protected:
    int verticalOffset() const override;
    int horizontalOffset() const override;
    void paintEvent(QPaintEvent * event) override;
    void updateGeometries() override;
    
private:
    bool tryToCalculateGeometriesWithNoScrollbars(const QStyleOptionViewItem& option, const QAbstractItemModel* model, uint32_t totalHeight);
    
private:
    struct Hint {
        bool dirty;
        uint32_t offset;
        uint32_t height;
    };
    std::deque<Hint> hints;
    int vo;
    
};

#endif //FEEDVIEW_H
