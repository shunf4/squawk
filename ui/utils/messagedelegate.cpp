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

#include <QPainter>
#include <QApplication>
#include "messagedelegate.h"
#include "ui/models/messagefeed.h"

constexpr int avatarHeight = 50;
constexpr int margin = 6;

MessageDelegate::MessageDelegate(QObject* parent):
QStyledItemDelegate(parent),
bodyFont(),
nickFont(),
dateFont(),
bodyMetrics(bodyFont),
nickMetrics(nickFont),
dateMetrics(dateFont)
{
}

MessageDelegate::~MessageDelegate()
{
}

void MessageDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    bool sentByMe = false;
    QVariant sbm = index.data(Models::MessageFeed::SentByMe);
    if (sbm.isValid()) {
        sentByMe = sbm.toBool();
    }
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    QIcon icon(index.data(Models::MessageFeed::Avatar).toString());
    
    if (sentByMe) {
        painter->drawPixmap(option.rect.width() - avatarHeight - margin,  option.rect.y() + margin / 2, icon.pixmap(avatarHeight, avatarHeight));
    } else {
        painter->drawPixmap(margin, option.rect.y() + margin / 2, icon.pixmap(avatarHeight, avatarHeight));
    }
    
    QStyleOptionViewItem opt = option;
    QRect messageRect = option.rect.adjusted(margin, margin / 2, -(avatarHeight + 2 * margin), -margin / 2);
    if (!sentByMe) {
        opt.displayAlignment = Qt::AlignLeft | Qt::AlignTop;
        messageRect.adjust(avatarHeight + margin, 0, avatarHeight + margin, 0);
    } else {
        opt.displayAlignment = Qt::AlignRight | Qt::AlignTop;
    }
    opt.rect = messageRect;
    
    QRect rect;
    painter->setFont(nickFont);
    painter->drawText(opt.rect, opt.displayAlignment, index.data(Models::MessageFeed::Sender).toString(), &rect);
    
    opt.rect.adjust(0, rect.height(), 0, 0);
    painter->setFont(bodyFont);
    painter->drawText(opt.rect, opt.displayAlignment | Qt::TextWordWrap, index.data(Models::MessageFeed::Text).toString(), &rect);
    
    opt.rect.adjust(0, rect.height(), 0, 0);
    painter->setFont(dateFont);
    QColor q = painter->pen().color();
    q.setAlpha(180);
    painter->setPen(q);
    painter->drawText(opt.rect, opt.displayAlignment, index.data(Models::MessageFeed::Date).toDateTime().toLocalTime().toString(), &rect);
    
    painter->restore();
}

QSize MessageDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QRect messageRect = option.rect.adjusted(0, margin / 2, -(avatarHeight + 3 * margin), -margin);
    QStyleOptionViewItem opt = option;
    opt.rect = messageRect;
    QSize messageSize = bodyMetrics.boundingRect(messageRect, Qt::TextWordWrap, index.data(Models::MessageFeed::Text).toString()).size();
    
    messageSize.rheight() += nickMetrics.lineSpacing();
    messageSize.rheight() += dateMetrics.height();
    
    if (messageSize.height() < avatarHeight) {
        messageSize.setHeight(avatarHeight);
    }
    
    messageSize.rheight() += margin;
    
    return messageSize;
}

void MessageDelegate::initializeFonts(const QFont& font)
{
    bodyFont = font;
    nickFont = font;
    dateFont = font;
    
    nickFont.setBold(true);
    dateFont.setItalic(true);
    float dps = dateFont.pointSizeF();
    if (dps != -1) {
        dateFont.setPointSizeF(dps * 0.7);
    } else {
        dateFont.setPointSize(dateFont.pointSize() - 2);
    }
    
    bodyMetrics = QFontMetrics(bodyFont);
    nickMetrics = QFontMetrics(nickFont);
    dateMetrics = QFontMetrics(dateFont);
}


// void MessageDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
// {
//     
// }
