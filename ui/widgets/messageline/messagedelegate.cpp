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
#include <QClipboard>
#include <QtMath>

#include "messagedelegate.h"
#include "messagefeed.h"

constexpr int avatarHeight = 50;
constexpr int margin = 6;
constexpr int textMargin = 2;
constexpr int statusIconSize = 16;
constexpr qreal messageMaxWidthRatio = 0.64;
constexpr int messageMinWidth = 420;
constexpr qreal inaccurateBodyMeasureHeightFix = 1.04;
constexpr qreal inaccurateBodyMeasureWidthFix = 1.08;
constexpr int bodyTextFlag = Qt::TextWordWrap | Qt::TextExpandTabs | Qt::TextDontClip | Qt::TextIncludeTrailingSpaces;

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
    downloadButtons(new std::map<QString, FeedButton*>()),
    copyLinkButtons(new std::map<QString, FeedButton*>()),
    bars(new std::map<QString, QProgressBar*>()),
    statusIcons(new std::map<QString, QLabel*>()),
    pencilIcons(new std::map<QString, QLabel*>()),
    bodies(new std::map<QString, QLabel*>()),
    previews(new std::map<QString, Preview*>()),
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
    for (const std::pair<const QString, FeedButton*>& pair: *downloadButtons){
        delete pair.second;
    }

    for (const std::pair<const QString, FeedButton*>& pair: *copyLinkButtons){
        delete pair.second;
    }
    
    for (const std::pair<const QString, QProgressBar*>& pair: *bars){
        delete pair.second;
    }
    
    for (const std::pair<const QString, QLabel*>& pair: *statusIcons){
        delete pair.second;
    }
    
    for (const std::pair<const QString, QLabel*>& pair: *pencilIcons){
        delete pair.second;
    }
    
    for (const std::pair<const QString, QLabel*>& pair: *bodies){
        delete pair.second;
    }
    
    for (const std::pair<const QString, Preview*>& pair: *previews){
        delete pair.second;
    }
    
    delete statusIcons;
    delete pencilIcons;
    delete idsToKeep;
    delete downloadButtons;
    delete copyLinkButtons;
    delete bars;
    delete bodies;
    delete previews;
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
    QRect origMessageRect = messageRect;
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
        QRect widthLimited;
        int widthAdjustment = qMin(
                    qRound(static_cast<qreal>(origMessageRect.width()) * (1 - messageMaxWidthRatio)),
                    origMessageRect.width() - qMin(messageMinWidth, origMessageRect.width()));
        if (!data.sentByMe) {
            widthLimited = messageRect.adjusted(0, 0, -widthAdjustment, 0);
        } else {
            widthLimited = messageRect.adjusted(+widthAdjustment, 0, 0, 0);
        }

        widthLimited.setWidth(qFloor(qreal(widthLimited.width())) / inaccurateBodyMeasureWidthFix);

        messageSize = bodyMetrics.boundingRect(widthLimited, bodyTextFlag, data.text).size();
        messageSize.rheight() = qCeil(qreal(messageSize.height()) * inaccurateBodyMeasureHeightFix);
        messageSize.rwidth() = qCeil(qreal(messageSize.width()) * inaccurateBodyMeasureWidthFix);
        bodySize = messageSize;
    }
    messageSize.rheight() += nickMetrics.lineSpacing();
    messageSize.rheight() += dateMetrics.height();
    QString dateString = data.date.toLocalTime().toString("hh:mm");
    if (messageSize.width() < opt.rect.width()) {
        QSize senderSize = nickMetrics.boundingRect(messageRect, 0, data.sender).size();
        if (senderSize.width() > messageSize.width()) {
            messageSize.setWidth(senderSize.width());
        }
        QSize dateSize = dateMetrics.boundingRect(messageRect, 0, dateString).size();
        int addition = 0;
        
        if (data.correction.corrected) {
            addition += margin + statusIconSize;
        }
        if (data.sentByMe) {
            addition += margin + statusIconSize;
        }
        if (dateSize.width() + addition > messageSize.width()) {
            messageSize.setWidth(dateSize.width() + addition);
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
            paintButtons(getDownloadButton(data), getCopyLinkButton(data), painter, data.sentByMe, opt);
            break;
        case Models::ready:
        case Models::local:
            clearHelperWidget(data);
            paintPreview(data, painter, opt);
            break;
        case Models::errorDownload: {
            paintButtons(getDownloadButton(data), getCopyLinkButton(data), painter, data.sentByMe, opt);
            paintComment(data, painter, opt);
        }
            
            break;
        case Models::errorUpload:{
            clearHelperWidget(data);
            paintPreview(data, painter, opt);
            paintComment(data, painter, opt);
        }
            break;
    }
    painter->restore();
    
    int messageLeft = INT16_MAX;
    int messageRight = opt.rect.x() + messageSize.width();
    QWidget* vp = static_cast<QWidget*>(painter->device());
    if (data.text.size() > 0) {
        QLabel* body = getBody(data);
        body->setParent(vp);
        body->setMaximumWidth(bodySize.width());
        body->setMinimumWidth(bodySize.width());
        body->setMinimumHeight(bodySize.height());
        body->setMaximumHeight(bodySize.height());
        body->setAlignment((opt.displayAlignment & ~Qt::AlignRight) | Qt::AlignLeft);
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
    painter->drawText(opt.rect, opt.displayAlignment, dateString, &rect);
    int currentY = opt.rect.y();
    if (data.sentByMe) {
        QLabel* statusIcon = getStatusIcon(data);
        
        statusIcon->setParent(vp);
        statusIcon->move(opt.rect.topRight().x() - messageSize.width(), currentY);
        statusIcon->show();
        
        opt.rect.adjust(0, statusIconSize + textMargin, 0, 0);
    }
    
    if (data.correction.corrected) {
        QLabel* pencilIcon = getPencilIcon(data);
        
        pencilIcon->setParent(vp);
        if (data.sentByMe) {
            pencilIcon->move(opt.rect.topRight().x() - messageSize.width() + statusIconSize + margin, currentY);
        } else {
            pencilIcon->move(messageRight - statusIconSize - margin, currentY);
        }
        pencilIcon->show();
    } else {
        std::map<QString, QLabel*>::const_iterator itr = pencilIcons->find(data.id);
        if (itr != pencilIcons->end()) {
            delete itr->second;
            pencilIcons->erase(itr);
        }
    }
    
    painter->restore();
    
    if (clearingWidgets) {
        idsToKeep->insert(data.id);
    }
}

QSize MessageDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QRect messageRect = option.rect.adjusted(0, margin / 2, -(avatarHeight + 3 * margin), -margin / 2);
    QRect origMessageRect = messageRect;
    QStyleOptionViewItem opt = option;
    opt.rect = messageRect;
    QVariant va = index.data(Models::MessageFeed::Attach);
    Models::Attachment attach = qvariant_cast<Models::Attachment>(va);
    QString body = index.data(Models::MessageFeed::Text).toString();
    bool sentByMe = index.data(Models::MessageFeed::SentByMe).toBool();
    QSize messageSize(0, 0);
    if (body.size() > 0) {
        QRect widthLimited;
        int widthAdjustment = qMin(
                    qRound(static_cast<qreal>(origMessageRect.width()) * (1 - messageMaxWidthRatio)),
                    origMessageRect.width() - qMin(messageMinWidth, origMessageRect.width()));
        if (!sentByMe) {
            widthLimited = messageRect.adjusted(0, 0, -widthAdjustment, 0);
        } else {
            widthLimited = messageRect.adjusted(+widthAdjustment, 0, 0, 0);
        }

        widthLimited.setWidth(qFloor(qreal(widthLimited.width())) / inaccurateBodyMeasureWidthFix);

        messageSize = bodyMetrics.boundingRect(widthLimited, bodyTextFlag, body).size();
        messageSize.rheight() = qCeil(qreal(messageSize.height()) * inaccurateBodyMeasureHeightFix);
        messageSize.rwidth() = qCeil(qreal(messageSize.width()) * inaccurateBodyMeasureWidthFix);
        messageSize.rheight() += textMargin;
    }
    
    switch (attach.state) {
        case Models::none:
            break;
        case Models::uploading:
            messageSize.rheight() += Preview::calculateAttachSize(attach.localPath, messageRect).height() + textMargin;
        case Models::downloading:
            messageSize.rheight() += barHeight + textMargin;
            break;
        case Models::remote:
            messageSize.rheight() += buttonHeight + textMargin;
            break;
        case Models::ready:
        case Models::local:
            messageSize.rheight() += Preview::calculateAttachSize(attach.localPath, messageRect).height() + textMargin;
            break;
        case Models::errorDownload:
            messageSize.rheight() += buttonHeight + textMargin;
            messageSize.rheight() += dateMetrics.boundingRect(messageRect, Qt::TextWordWrap, attach.error).size().height() + textMargin;
            break;
        case Models::errorUpload:
            messageSize.rheight() += Preview::calculateAttachSize(attach.localPath, messageRect).height() + textMargin;
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
    
    bodyFont.setFamilies(QStringList("Microsoft YaHei"));
    bodyMetrics = QFontMetrics(bodyFont);
    nickMetrics = QFontMetrics(nickFont);
    dateMetrics = QFontMetrics(dateFont);
    
    Preview::initializeFont(bodyFont);
}

bool MessageDelegate::editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index)
{
    //qDebug() << event->type();
    
    
    return QStyledItemDelegate::editorEvent(event, model, option, index);
}

void MessageDelegate::paintButtons(QPushButton* btn1, QPushButton* btn2, QPainter* painter, bool sentByMe, QStyleOptionViewItem& option) const
{
    static const int buttonGap = 10;
    QPoint start;
    if (sentByMe) {
        start = {option.rect.width() - btn1->width() - buttonGap - btn2->width(), option.rect.top()};
    } else {
        start = option.rect.topLeft();
    }
    
    QWidget* vp = static_cast<QWidget*>(painter->device());
    btn1->setParent(vp);
    btn1->move(start);
    btn1->show();
    btn2->setParent(vp);
    btn2->move({start.x() + buttonGap + btn1->width(), start.y()});
    btn2->show();
    
    option.rect.adjust(0, buttonHeight + textMargin, 0, 0);
}

void MessageDelegate::paintComment(const Models::FeedItem& data, QPainter* painter, QStyleOptionViewItem& option) const
{
    painter->setFont(dateFont);
    QColor q = painter->pen().color();
    q.setAlpha(180);
    painter->setPen(q);
    QRect rect;
    painter->drawText(option.rect, option.displayAlignment, data.attach.error, &rect);
    option.rect.adjust(0, rect.height() + textMargin, 0, 0);
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
    Preview* preview = 0;
    std::map<QString, Preview*>::iterator itr = previews->find(data.id);

    QSize size = option.rect.size();
    if (itr != previews->end()) {
        preview = itr->second;
        preview->actualize(data.attach.localPath, size, option.rect.topLeft());
    } else {
        QWidget* vp = static_cast<QWidget*>(painter->device());
        preview = new Preview(data.attach.localPath, size, option.rect.topLeft(), data.sentByMe, vp);
        previews->insert(std::make_pair(data.id, preview));
    }
    
    if (!preview->isFileReachable()) {      //this is the situation when the file preview couldn't be painted because the file was moved 
        emit invalidPath(data.id);          //or deleted. This signal notifies the model, and the model notifies the core, preview can 
    }                                       //handle being invalid for as long as I need and can be even become valid again with a new path
    
    option.rect.adjust(0, preview->size().height() + textMargin, 0, 0);
}

QPushButton * MessageDelegate::getDownloadButton(const Models::FeedItem& data) const
{
    std::map<QString, FeedButton*>::const_iterator itr = downloadButtons->find(data.id);
    FeedButton* result = 0;
    if (itr != downloadButtons->end()) {
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
        downloadButtons->insert(std::make_pair(data.id, result));
        connect(result, &QPushButton::clicked, this, &MessageDelegate::onButtonPushed);
    }
    
    return result;
}

QPushButton * MessageDelegate::getCopyLinkButton(const Models::FeedItem& data) const
{
    std::map<QString, FeedButton*>::const_iterator itr = copyLinkButtons->find(data.id);
    FeedButton* result = 0;
    if (itr != copyLinkButtons->end()) {
        result = itr->second;
    }

    if (result == 0) {
        const QString& remotePath = data.attach.remotePath;

        result = new FeedButton();
        result->setToolTip(remotePath);
        result->messageId = data.id;
        result->setText(QCoreApplication::translate("MessageLine", "Copy Link"));
        copyLinkButtons->insert(std::make_pair(data.id, result));

        connect(result, &QPushButton::clicked, this, [=](){
            QApplication::clipboard()->setText(remotePath);
        });
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
        std::map<QString, FeedButton*>::const_iterator itr = downloadButtons->find(data.id);
        if (itr != downloadButtons->end()) {
            delete itr->second;
            downloadButtons->erase(itr);
        }

        auto clbItr = copyLinkButtons->find(data.id);
        if (clbItr != copyLinkButtons->end()) {
            delete clbItr->second;
            copyLinkButtons->erase(clbItr);
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
    
    if (itr != statusIcons->end()) {
        result = itr->second;
    } else {
        result = new QLabel();
        statusIcons->insert(std::make_pair(data.id, result));
    }
    
    QIcon q(Shared::icon(Shared::messageStateThemeIcons[static_cast<uint8_t>(data.state)]));
    QString tt = Shared::Global::getName(data.state);
    if (data.state == Shared::Message::State::error) {
        if (data.error > 0) {
            tt += ": " + data.error;
        }
    }
    if (result->toolTip() != tt) {                      //If i just assign pixmap every time unconditionally
        result->setPixmap(q.pixmap(statusIconSize));    //it invokes an infinite cycle of repaint
        result->setToolTip(tt);                         //may be it's better to subclass and store last condition in int?
    }
    
    return result;
}

QLabel * MessageDelegate::getPencilIcon(const Models::FeedItem& data) const
{
    std::map<QString, QLabel*>::const_iterator itr = pencilIcons->find(data.id);
    QLabel* result = 0;
    
    if (itr != pencilIcons->end()) {
        result = itr->second;
    } else {
        result = new QLabel();
        QIcon icon = Shared::icon("edit-rename");
        result->setPixmap(icon.pixmap(statusIconSize));
        pencilIcons->insert(std::make_pair(data.id, result));
    }
    
    result->setToolTip("Last time edited: " + data.correction.lastCorrection.toLocalTime().toString() 
    + "\nOriginal message: " + data.correction.original);
    
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

template <typename T>
void removeElements(std::map<QString, T*>* elements, std::set<QString>* idsToKeep) {
    std::set<QString> toRemove;
    for (const std::pair<const QString, T*>& pair: *elements) {
        if (idsToKeep->find(pair.first) == idsToKeep->end()) {
            delete pair.second;
            toRemove.insert(pair.first);
        }
    }
    for (const QString& key : toRemove) {
        elements->erase(key);
    }
}

void MessageDelegate::endClearWidgets()
{
    if (clearingWidgets) {
        removeElements(downloadButtons, idsToKeep);
        removeElements(copyLinkButtons, idsToKeep);
        removeElements(bars, idsToKeep);
        removeElements(statusIcons, idsToKeep);
        removeElements(pencilIcons, idsToKeep);
        removeElements(bodies, idsToKeep);
        removeElements(previews, idsToKeep);
        
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
    auto clbItr = copyLinkButtons->find(data.id);
    if (clbItr != copyLinkButtons->end()) {
        delete clbItr->second;
        copyLinkButtons->erase(clbItr);
    }

    std::map<QString, FeedButton*>::const_iterator itr = downloadButtons->find(data.id);
    if (itr != downloadButtons->end()) {
        delete itr->second;
        downloadButtons->erase(itr);
    } else {
        std::map<QString, QProgressBar*>::const_iterator barItr = bars->find(data.id);
        if (barItr != bars->end()) {
            delete barItr->second;
            bars->erase(barItr);
        }
    }
}

// void MessageDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
// {
//     
// }
