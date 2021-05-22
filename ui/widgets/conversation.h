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

#ifndef CONVERSATION_H
#define CONVERSATION_H

#include <QWidget>
#include <QScopedPointer>
#include <QMap>
#include <QMimeData>
#include <QFileInfo>
#include <QGraphicsOpacityEffect>
#include <QMenu>
#include <QAction>
#include <QDesktopServices>

#include "shared/message.h"
#include "shared/order.h"
#include "shared/icons.h"
#include "shared/utils.h"

#include "ui/models/account.h"
#include "ui/models/roster.h"

#include "ui/utils/flowlayout.h"
#include "ui/utils/badge.h"
#include "ui/utils/shadowoverlay.h"

#include "ui/widgets/messageline/feedview.h"
#include "ui/widgets/messageline/messagedelegate.h"

namespace Ui
{
class Conversation;
}

class KeyEnterReceiver : public QObject
{
    Q_OBJECT
public:
    KeyEnterReceiver(QObject* parent = 0);
protected:
    bool ownEvent;
    bool eventFilter(QObject* obj, QEvent* event);
    
signals:
    void enterPressed();
};

class Conversation : public QWidget
{
    Q_OBJECT
public:
    Conversation(bool muc, Models::Account* acc, Models::Element* el, const QString pJid, const QString pRes, QWidget* parent = 0);
    ~Conversation();
    
    QString getJid() const;
    QString getAccount() const;
    QString getPalResource() const;
    Models::Roster::ElId getId() const;
    
    void setPalResource(const QString& res);
    virtual void setAvatar(const QString& path);
    void setFeedFrames(bool top, bool right, bool bottom, bool left);
    
signals:
    void sendMessage(const Shared::Message& message);
    void resendMessage(const QString& id);
    void requestArchive(const QString& before);
    void shown();
    void requestLocalFile(const QString& messageId, const QString& url);
    void downloadFile(const QString& messageId, const QString& url);
    void notifyableMessage(const QString& account, const Shared::Message& msg);
    
protected:
    virtual void setName(const QString& name);
    virtual Shared::Message createMessage() const;
    void setStatus(const QString& status);
    void addAttachedFile(const QString& path);
    void removeAttachedFile(Badge* badge);
    void clearAttachedFiles();
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragLeaveEvent(QDragLeaveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void initializeOverlay();
    virtual void onMessage(const Shared::Message& msg);
    
protected slots:
    void onEnterPressed();
    void onAttach();
    void onFileSelected();
    void onBadgeClose();
    void onClearButton();
    void onTextEditDocSizeChanged(const QSizeF& size);
    void onAccountChanged(Models::Item* item, int row, int col);
    void onFeedMessage(const Shared::Message& msg);
    void positionShadow();
    void onFeedContext(const QPoint &pos);
    
public:
    const bool isMuc;
    
protected:
    Models::Account* account;
    Models::Element* element;
    QString palJid;
    QString activePalResource;
    QScopedPointer<Ui::Conversation> m_ui;
    KeyEnterReceiver ker;
    QString thread;
    QLabel* statusIcon;
    QLabel* statusLabel;
    FlowLayout* filesLayout;
    QWidget* overlay;
    W::Order<Badge*, Badge::Comparator> filesToAttach;
    FeedView* feed;
    MessageDelegate* delegate;
    bool manualSliderChange;
    bool tsb;           //transient scroll bars
    
    ShadowOverlay shadow;
    QMenu* contextMenu;
};

#endif // CONVERSATION_H
