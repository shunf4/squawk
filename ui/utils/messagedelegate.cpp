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

#include <QDebug>
#include <QPainter>
#include <QApplication>
#include <QMouseEvent>

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
dateMetrics(dateFont),
buttonHeight(0),
buttons(new std::map<QString, FeedButton*>()),
idsToKeep(new std::set<QString>()),
clearingWidgets(false)
{
    QPushButton btn;
    buttonHeight = btn.sizeHint().height();
}

MessageDelegate::~MessageDelegate()
{
    for (const std::pair<const QString, FeedButton*>& pair: *buttons){
        delete pair.second;
    }
    
    delete idsToKeep;
    delete buttons;
}

void MessageDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QVariant vi = index.data(Models::MessageFeed::Bulk);
    if (!vi.isValid()) {
        return;
    }
    Models::FeedItem data = qvariant_cast<Models::FeedItem>(vi);
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    
    if (option.state & QStyle::State_MouseOver) {
        painter->fillRect(option.rect, option.palette.brush(QPalette::Inactive, QPalette::Highlight));
    }
    
    QIcon icon(data.avatar);
    
    if (data.sentByMe) {
        painter->drawPixmap(option.rect.width() - avatarHeight - margin,  option.rect.y() + margin / 2, icon.pixmap(avatarHeight, avatarHeight));
    } else {
        painter->drawPixmap(margin, option.rect.y() + margin / 2, icon.pixmap(avatarHeight, avatarHeight));
    }
    
    QStyleOptionViewItem opt = option;
    QRect messageRect = option.rect.adjusted(margin, margin / 2, -(avatarHeight + 2 * margin), -margin / 2);
    if (!data.sentByMe) {
        opt.displayAlignment = Qt::AlignLeft | Qt::AlignTop;
        messageRect.adjust(avatarHeight + margin, 0, avatarHeight + margin, 0);
    } else {
        opt.displayAlignment = Qt::AlignRight | Qt::AlignTop;
    }
    opt.rect = messageRect;
    
    QSize messageSize(0, 0);
    if (data.text.size() > 0) {
        messageSize = bodyMetrics.boundingRect(messageRect, Qt::TextWordWrap, data.text).size();
    }
    messageSize.rheight() += nickMetrics.lineSpacing();
    messageSize.rheight() += dateMetrics.height();
    if (messageSize.width() < opt.rect.width()) {
        QSize senderSize = nickMetrics.boundingRect(messageRect, 0, data.sender).size();
        if (senderSize.width() > messageSize.width()) {
            messageSize.setWidth(senderSize.width());
        }
    } else {
        messageSize.setWidth(opt.rect.width());
    }
    
    QRect rect;
    painter->setFont(nickFont);
    painter->drawText(opt.rect, opt.displayAlignment, data.sender, &rect);
    
    
    opt.rect.adjust(0, rect.height(), 0, 0);
    painter->save();
    switch (data.attach.state) {
        case Models::none:
            break;
        case Models::uploading:
        case Models::downloading:
            break;
        case Models::remote:
        case Models::local:
            paintButton(getButton(data), painter, data.sentByMe, opt);
            break;
        case Models::ready:
            break;
    }
    painter->restore();
    
    if (data.text.size() > 0) {
        painter->setFont(bodyFont);
        painter->drawText(opt.rect, opt.displayAlignment | Qt::TextWordWrap, data.text, &rect);
        opt.rect.adjust(0, rect.height(), 0, 0);
    }
    painter->setFont(dateFont);
    QColor q = painter->pen().color();
    q.setAlpha(180);
    painter->setPen(q);
    painter->drawText(opt.rect, opt.displayAlignment, data.date.toLocalTime().toString(), &rect);
    
    painter->restore();
    
    if (clearingWidgets) {
        idsToKeep->insert(data.id);
    }
}

QSize MessageDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QRect messageRect = option.rect.adjusted(0, margin / 2, -(avatarHeight + 3 * margin), -margin / 2);
    QStyleOptionViewItem opt = option;
    opt.rect = messageRect;
    QVariant va = index.data(Models::MessageFeed::Attach);
    Models::Attachment attach = qvariant_cast<Models::Attachment>(va);
    QString body = index.data(Models::MessageFeed::Text).toString();
    QSize messageSize(0, 0);
    if (body.size() > 0) {
        messageSize = bodyMetrics.boundingRect(messageRect, Qt::TextWordWrap, body).size();
    }
    
    switch (attach.state) {
        case Models::none:
            break;
        case Models::uploading:
        case Models::downloading:
            break;
        case Models::remote:
        case Models::local:
            messageSize.rheight() += buttonHeight;
            break;
        case Models::ready:
            break;
    }
    
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

bool MessageDelegate::editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index)
{
    //qDebug() << event->type();
    
    
    return QStyledItemDelegate::editorEvent(event, model, option, index);
}

void MessageDelegate::paintButton(QPushButton* btn, QPainter* painter, bool sentByMe, QStyleOptionViewItem& option) const
{
    QPoint start;
    if (sentByMe) {
        start = {option.rect.width() - btn->width(), option.rect.top()};
    } else {
        start = option.rect.topLeft();
    }
    
    QWidget* vp = static_cast<QWidget*>(painter->device());
    btn->setParent(vp);
    btn->move(start);
    btn->show();
    
    option.rect.adjust(0, buttonHeight, 0, 0);
}


QPushButton * MessageDelegate::getButton(const Models::FeedItem& data) const
{
    std::map<QString, FeedButton*>::const_iterator itr = buttons->find(data.id);
    FeedButton* result = 0;
    if (itr != buttons->end()) {
        if (
            (data.attach.state == Models::remote && itr->second->download) ||
            (data.attach.state == Models::local && !itr->second->download)
        ) {
            result = itr->second;
        } else {
            delete itr->second;
            buttons->erase(itr);
        }
    }
    
    if (result == 0) {
        result = new FeedButton();
        result->messageId = data.id;
        if (data.attach.state == Models::remote) {
            result->setText(QCoreApplication::translate("MessageLine", "Download"));
            result->download = true;
        } else {
            result->setText(QCoreApplication::translate("MessageLine", "Upload"));
            result->download = false;
        }
        buttons->insert(std::make_pair(data.id, result));
    }
    
    return result;
}


void MessageDelegate::beginClearWidgets()
{
    idsToKeep->clear();
    clearingWidgets = true;
}

void MessageDelegate::endClearWidgets()
{
    if (clearingWidgets) {
        std::set<QString> toRemove;
        for (const std::pair<const QString, FeedButton*>& pair: *buttons){
            if (idsToKeep->find(pair.first) == idsToKeep->end()) {
                delete pair.second;
                toRemove.insert(pair.first);
            }
        }
        
        for (const QString& key : toRemove) {
            buttons->erase(key);
        }
        
        idsToKeep->clear();
        clearingWidgets = false;
    }
}


// void MessageDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
// {
//     
// }
