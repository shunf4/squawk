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

#ifndef MESSAGEDELEGATE_H
#define MESSAGEDELEGATE_H

#include <map>
#include <set>

#include <QStyledItemDelegate>
#include <QStyleOptionButton>
#include <QFont>
#include <QFontMetrics>
#include <QPushButton>
#include <QProgressBar>
#include <QLabel>

#include "shared/icons.h"
#include "shared/global.h"
#include "shared/utils.h"

#include "preview.h"

namespace Models {
    struct FeedItem;
};

class MessageDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    MessageDelegate(QObject *parent = nullptr);
    ~MessageDelegate();
    
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    //void setModelData(QWidget * editor, QAbstractItemModel * model, const QModelIndex & index) const override;
    
    void initializeFonts(const QFont& font);
    bool editorEvent(QEvent * event, QAbstractItemModel * model, const QStyleOptionViewItem & option, const QModelIndex & index) override;
    void endClearWidgets();
    void beginClearWidgets();
    
signals:
    void buttonPushed(const QString& messageId) const;
    void invalidPath(const QString& messageId) const;
    
protected:
    void paintButton(QPushButton* btn, QPainter* painter, bool sentByMe, QStyleOptionViewItem& option) const;
    void paintBar(QProgressBar* bar, QPainter* painter, bool sentByMe, QStyleOptionViewItem& option) const;
    void paintPreview(const Models::FeedItem& data, QPainter* painter, QStyleOptionViewItem& option) const;
    void paintComment(const Models::FeedItem& data, QPainter* painter, QStyleOptionViewItem& option) const;
    QPushButton* getButton(const Models::FeedItem& data) const;
    QProgressBar* getBar(const Models::FeedItem& data) const;
    QLabel* getStatusIcon(const Models::FeedItem& data) const;
    QLabel* getBody(const Models::FeedItem& data) const;
    void clearHelperWidget(const Models::FeedItem& data) const;
    
protected slots:
    void onButtonPushed() const;
    
private:
    class FeedButton : public QPushButton {
    public:
        QString messageId;
    };
    
    QFont bodyFont;
    QFont nickFont;
    QFont dateFont;
    QFontMetrics bodyMetrics;
    QFontMetrics nickMetrics;
    QFontMetrics dateMetrics;
    
    int buttonHeight;
    int barHeight;
    
    std::map<QString, FeedButton*>* buttons;
    std::map<QString, QProgressBar*>* bars;
    std::map<QString, QLabel*>* statusIcons;
    std::map<QString, QLabel*>* bodies;
    std::map<QString, Preview*>* previews;
    std::set<QString>* idsToKeep;
    bool clearingWidgets;
    
};

#endif // MESSAGEDELEGATE_H
