/*
 * Squawk messenger. 
 * Copyright (C) 2019 Yury Gubich <blue@macaw.me>
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

#ifndef MESSAGE_H
#define MESSAGE_H

#include <QWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QGraphicsDropShadowEffect>
#include <QPushButton>
#include <QProgressBar>
#include <QAction>
#include <QDesktopServices>
#include <QUrl>
#include <QMap>

#include "shared/message.h"
#include "shared/icons.h"
#include "shared/global.h"
#include "shared/utils.h"
#include "resizer.h"
#include "image.h"

/**
 * @todo write docs
 */
class Message : public QWidget
{
    Q_OBJECT
public:
    Message(const Shared::Message& source, bool outgoing, const QString& sender, const QString& avatarPath = "", QWidget* parent = nullptr);
    ~Message();
    
    void setSender(const QString& sender);
    QString getId() const;
    QString getSenderJid() const;
    QString getSenderResource() const;
    QString getFileUrl() const;
    const Shared::Message& getMessage() const;
    
    void addButton(const QIcon& icon, const QString& buttonText, const QString& tooltip = "");
    void showComment(const QString& comment, bool wordWrap = false);
    void hideComment();
    void showFile(const QString& path);
    void setProgress(qreal value);
    void setAvatarPath(const QString& p_path);
    bool change(const QMap<QString, QVariant>& data);
    
    bool const outgoing;
    
signals:
    void buttonClicked();
    
private:
    Shared::Message msg;
    QWidget* body;
    QWidget* statusBar;
    QVBoxLayout* bodyLayout;
    QHBoxLayout* layout;
    QLabel* date;
    QLabel* sender;
    QLabel* text;
    QGraphicsDropShadowEffect* shadow;
    QPushButton* button;
    QLabel* file;
    QProgressBar* progress;
    QLabel* fileComment;
    QLabel* statusIcon;
    QLabel* editedLabel;
    Image* avatar;
    bool hasButton;
    bool hasProgress;
    bool hasFile;
    bool commentAdded;
    bool hasStatusIcon;
    bool hasEditedLabel;
  
private:
    void hideButton();
    void hideProgress();
    void hideFile();
    void setState();
    void setEdited();
};

#endif // MESSAGE_H
