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
constexpr int textMargin = 2;
constexpr int statusIconSize = 16;
constexpr int maxAttachmentHeight = 500;

MessageDelegate::MessageDelegate(QObject* parent):
    QStyledItemDelegate(parent),
    bodyFont(),
    nickFont(),
    dateFont(),
    bodyMetrics(bodyFont),
    nickMetrics(nickFont),
    dateMetrics(dateFont),
    buttonHeight(0),
    barHeight(0),
    buttons(new std::map<QString, FeedButton*>()),
    bars(new std::map<QString, QProgressBar*>()),
    statusIcons(new std::map<QString, QLabel*>()),
    bodies(new std::map<QString, QLabel*>()),
    idsToKeep(new std::set<QString>()),
    clearingWidgets(false)
{
    QPushButton btn;
    buttonHeight = btn.sizeHint().height();
    
    QProgressBar bar;
    barHeight = bar.sizeHint().height();
}

MessageDelegate::~MessageDelegate()
{
    for (const std::pair<const QString, FeedButton*>& pair: *buttons){
        delete pair.second;
    }
    
    for (const std::pair<const QString, QProgressBar*>& pair: *bars){
        delete pair.second;
    }
    
    for (const std::pair<const QString, QLabel*>& pair: *statusIcons){
        delete pair.second;
    }
    
    for (const std::pair<const QString, QLabel*>& pair: *bodies){
        delete pair.second;
    }
    
    delete idsToKeep;
    delete buttons;
    delete bars;
    delete bodies;
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
    QSize bodySize(0, 0);
    if (data.text.size() > 0) {
        messageSize = bodyMetrics.boundingRect(messageRect, Qt::TextWordWrap, data.text).size();
        bodySize = messageSize;
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
    opt.rect.adjust(0, rect.height() + textMargin, 0, 0);
    painter->save();
    switch (data.attach.state) {
        case Models::none:
            clearHelperWidget(data);        //i can't imagine the situation where it's gonna be needed
            break;                          //but it's a possible performance problem
        case Models::uploading:
            paintPreview(data, painter, opt);
        case Models::downloading:
            paintBar(getBar(data), painter, data.sentByMe, opt);
            break;
        case Models::remote:
            paintButton(getButton(data), painter, data.sentByMe, opt);
            break;
        case Models::ready:
        case Models::local:
            clearHelperWidget(data);
            paintPreview(data, painter, opt);
            break;
        case Models::errorDownload: {
            paintButton(getButton(data), painter, data.sentByMe, opt);
            painter->setFont(dateFont);
            QColor q = painter->pen().color();
            q.setAlpha(180);
            painter->setPen(q);
            painter->drawText(opt.rect, opt.displayAlignment, data.attach.error, &rect);
            opt.rect.adjust(0, rect.height() + textMargin, 0, 0);
        }
            
            break;
        case Models::errorUpload:{
            clearHelperWidget(data);
            paintPreview(data, painter, opt);
            painter->setFont(dateFont);
            QColor q = painter->pen().color();
            q.setAlpha(180);
            painter->setPen(q);
            painter->drawText(opt.rect, opt.displayAlignment, data.attach.error, &rect);
            opt.rect.adjust(0, rect.height() + textMargin, 0, 0);
        }
            break;
    }
    painter->restore();
    
    int messageLeft = INT16_MAX;
    QWidget* vp = static_cast<QWidget*>(painter->device());
    if (data.text.size() > 0) {
        QLabel* body = getBody(data);
        body->setParent(vp);
        body->setMaximumWidth(bodySize.width());
        body->setMinimumWidth(bodySize.width());
        body->setAlignment(opt.displayAlignment);
        messageLeft = opt.rect.x();
        if (data.sentByMe) {
            messageLeft = opt.rect.topRight().x() - bodySize.width();
        }
        body->move(messageLeft, opt.rect.y());
        body->show();
        opt.rect.adjust(0, bodySize.height() + textMargin, 0, 0);
    }
    painter->setFont(dateFont);
    QColor q = painter->pen().color();
    q.setAlpha(180);
    painter->setPen(q);
    painter->drawText(opt.rect, opt.displayAlignment, data.date.toLocalTime().toString(), &rect);
    if (data.sentByMe) {
        if (messageLeft > rect.x() - statusIconSize - margin) {
            messageLeft = rect.x() - statusIconSize - margin;
        }
        QLabel* statusIcon = getStatusIcon(data);
        
        statusIcon->setParent(vp);
        statusIcon->move(messageLeft, opt.rect.y());
        statusIcon->show();
        opt.rect.adjust(0, statusIconSize + textMargin, 0, 0);
    }
    
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
        messageSize.rheight() += textMargin;
    }
    
    switch (attach.state) {
        case Models::none:
            break;
        case Models::uploading:
            messageSize.rheight() += calculateAttachSize(attach.localPath, messageRect).height() + textMargin;
        case Models::downloading:
            messageSize.rheight() += barHeight + textMargin;
            break;
        case Models::remote:
            messageSize.rheight() += buttonHeight + textMargin;
            break;
        case Models::ready:
        case Models::local:
            messageSize.rheight() += calculateAttachSize(attach.localPath, messageRect).height() + textMargin;
            break;
        case Models::errorDownload:
            messageSize.rheight() += buttonHeight + textMargin;
            messageSize.rheight() += dateMetrics.boundingRect(messageRect, Qt::TextWordWrap, attach.error).size().height() + textMargin;
            break;
        case Models::errorUpload:
            messageSize.rheight() += calculateAttachSize(attach.localPath, messageRect).height() + textMargin;
            messageSize.rheight() += dateMetrics.boundingRect(messageRect, Qt::TextWordWrap, attach.error).size().height() + textMargin;
            break;
    }
    
    messageSize.rheight() += nickMetrics.lineSpacing();
    messageSize.rheight() += textMargin;
    messageSize.rheight() += dateMetrics.height() > statusIconSize ? dateMetrics.height() : statusIconSize;
    
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
    
    float ndps = nickFont.pointSizeF();
    if (ndps != -1) {
        nickFont.setPointSizeF(ndps * 1.2);
    } else {
        nickFont.setPointSize(nickFont.pointSize() + 2);
    }
    
    dateFont.setItalic(true);
    float dps = dateFont.pointSizeF();
    if (dps != -1) {
        dateFont.setPointSizeF(dps * 0.8);
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
    
    option.rect.adjust(0, buttonHeight + textMargin, 0, 0);
}

void MessageDelegate::paintBar(QProgressBar* bar, QPainter* painter, bool sentByMe, QStyleOptionViewItem& option) const
{
    QPoint start = option.rect.topLeft();
    bar->resize(option.rect.width(), barHeight);   
    
    painter->translate(start);
    bar->render(painter, QPoint(), QRegion(), QWidget::DrawChildren);
    
    option.rect.adjust(0, barHeight + textMargin, 0, 0);
}

void MessageDelegate::paintPreview(const Models::FeedItem& data, QPainter* painter, QStyleOptionViewItem& option) const
{
    Shared::Global::FileInfo info = Shared::Global::getFileInfo(data.attach.localPath);
    QSize size = constrainAttachSize(info.size, option.rect.size());
    
    QPoint start;
    if (data.sentByMe) {
        start = {option.rect.width() - size.width(), option.rect.top()};
        start.rx() += margin;
    } else {
        start = option.rect.topLeft();
    }
    QRect rect(start, size);
    switch (info.preview) {
        case Shared::Global::FileInfo::Preview::picture: {
            QImage img(data.attach.localPath);
            if (img.isNull()) {
                emit invalidPath(data.id);
            } else {
                painter->drawImage(rect, img);
            }
        }
        break;
        default: {
            QIcon icon = QIcon::fromTheme(info.mime.iconName());
            
            painter->save();
            
            painter->setFont(bodyFont);
            int labelWidth = option.rect.width() - size.width() - margin;
            QString elidedName = bodyMetrics.elidedText(info.name, Qt::ElideMiddle, labelWidth);
            QSize nameSize = bodyMetrics.boundingRect(QRect(start, QSize(labelWidth, 0)), 0, elidedName).size();
            if (data.sentByMe) {
                start.rx() -= nameSize.width() + margin;
            }
            painter->drawPixmap({start, size}, icon.pixmap(info.size));
            start.rx() += size.width() + margin;
            start.ry() += nameSize.height() + (size.height() - nameSize.height()) / 2;
            painter->drawText(start, elidedName);
    
            painter->restore();
        }
    }
    
    option.rect.adjust(0, size.height() + textMargin, 0, 0);
}

QPushButton * MessageDelegate::getButton(const Models::FeedItem& data) const
{
    std::map<QString, FeedButton*>::const_iterator itr = buttons->find(data.id);
    FeedButton* result = 0;
    if (itr != buttons->end()) {
        result = itr->second;
    } else {
        std::map<QString, QProgressBar*>::const_iterator barItr = bars->find(data.id);
        if (barItr != bars->end()) {
            delete barItr->second;
            bars->erase(barItr);
        }
    }
    
    if (result == 0) {
        result = new FeedButton();
        result->messageId = data.id;
        result->setText(QCoreApplication::translate("MessageLine", "Download"));
        buttons->insert(std::make_pair(data.id, result));
        connect(result, &QPushButton::clicked, this, &MessageDelegate::onButtonPushed);
    }
    
    return result;
}

QProgressBar * MessageDelegate::getBar(const Models::FeedItem& data) const
{
    std::map<QString, QProgressBar*>::const_iterator barItr = bars->find(data.id);
    QProgressBar* result = 0;
    if (barItr != bars->end()) {
        result = barItr->second;
    } else {
        std::map<QString, FeedButton*>::const_iterator itr = buttons->find(data.id);
        if (itr != buttons->end()) {
            delete itr->second;
            buttons->erase(itr);
        }
    }
    
    if (result == 0) {
        result = new QProgressBar();
        result->setRange(0, 100);
        bars->insert(std::make_pair(data.id, result));
    }
    
    result->setValue(data.attach.progress * 100);
    
    return result;
}

QLabel * MessageDelegate::getStatusIcon(const Models::FeedItem& data) const
{
    std::map<QString, QLabel*>::const_iterator itr = statusIcons->find(data.id);
    QLabel* result = 0;
    
    QIcon q(Shared::icon(Shared::messageStateThemeIcons[static_cast<uint8_t>(data.state)]));
    QString tt = Shared::Global::getName(data.state);
    if (data.state == Shared::Message::State::error) {
        if (data.error > 0) {
            tt += ": " + data.error;
        }
    }
    
    if (itr != statusIcons->end()) {
        result = itr->second;
        if (result->toolTip() != tt) {                      //If i just assign pixmap every time unconditionally
            result->setPixmap(q.pixmap(statusIconSize));    //it involves into an infinite cycle of repaint
            result->setToolTip(tt);                         //may be it's better to subclass and store last condition in int?
        }
    } else {
        result = new QLabel();
        statusIcons->insert(std::make_pair(data.id, result));
        result->setPixmap(q.pixmap(statusIconSize));
        result->setToolTip(tt);
    }
    
    
    
    result->setToolTip(tt);
    //result->setText(std::to_string((int)data.state).c_str());
    
    return result;
}

QLabel * MessageDelegate::getBody(const Models::FeedItem& data) const
{
    std::map<QString, QLabel*>::const_iterator itr = bodies->find(data.id);
    QLabel* result = 0;
    
    if (itr != bodies->end()) {
        result = itr->second;
    } else {
        result = new QLabel();
        result->setFont(bodyFont);
        result->setWordWrap(true);
        result->setOpenExternalLinks(true);
        result->setTextInteractionFlags(result->textInteractionFlags() | Qt::TextSelectableByMouse | Qt::LinksAccessibleByMouse);
        bodies->insert(std::make_pair(data.id, result));
    }
    
    result->setText(Shared::processMessageBody(data.text));
    
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
        std::set<QString> toRemoveButtons;
        std::set<QString> toRemoveBars;
        std::set<QString> toRemoveIcons;
        std::set<QString> toRemoveBodies;
        for (const std::pair<const QString, FeedButton*>& pair: *buttons) {
            if (idsToKeep->find(pair.first) == idsToKeep->end()) {
                delete pair.second;
                toRemoveButtons.insert(pair.first);
            }
        }
        for (const std::pair<const QString, QProgressBar*>& pair: *bars) {
            if (idsToKeep->find(pair.first) == idsToKeep->end()) {
                delete pair.second;
                toRemoveBars.insert(pair.first);
            }
        }
        for (const std::pair<const QString, QLabel*>& pair: *statusIcons) {
            if (idsToKeep->find(pair.first) == idsToKeep->end()) {
                delete pair.second;
                toRemoveIcons.insert(pair.first);
            }
        }
        for (const std::pair<const QString, QLabel*>& pair: *bodies) {
            if (idsToKeep->find(pair.first) == idsToKeep->end()) {
                delete pair.second;
                toRemoveBodies.insert(pair.first);
            }
        }
        
        for (const QString& key : toRemoveButtons) {
            buttons->erase(key);
        }
        for (const QString& key : toRemoveBars) {
            bars->erase(key);
        }
        for (const QString& key : toRemoveIcons) {
            statusIcons->erase(key);
        }
        for (const QString& key : toRemoveBodies) {
            bodies->erase(key);
        }
        
        idsToKeep->clear();
        clearingWidgets = false;
    }
}

void MessageDelegate::onButtonPushed() const
{
    FeedButton* btn = static_cast<FeedButton*>(sender());
    emit buttonPushed(btn->messageId);
}

void MessageDelegate::clearHelperWidget(const Models::FeedItem& data) const
{
    std::map<QString, FeedButton*>::const_iterator itr = buttons->find(data.id);
    if (itr != buttons->end()) {
        delete itr->second;
        buttons->erase(itr);
    } else {
        std::map<QString, QProgressBar*>::const_iterator barItr = bars->find(data.id);
        if (barItr != bars->end()) {
            delete barItr->second;
            bars->erase(barItr);
        }
    }
}

QSize MessageDelegate::calculateAttachSize(const QString& path, const QRect& bounds) const
{
    Shared::Global::FileInfo info = Shared::Global::getFileInfo(path);
    
    return constrainAttachSize(info.size, bounds.size());
}

QSize MessageDelegate::constrainAttachSize(QSize src, QSize bounds) const
{
    bounds.setHeight(maxAttachmentHeight);
    
    if (src.width() > bounds.width() || src.height() > bounds.height()) {
        src.scale(bounds, Qt::KeepAspectRatio);
    }
    
    return src;
}


// void MessageDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
// {
//     
// }
