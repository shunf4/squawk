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

#include "global.h"
#include "resizer.h"
#include "image.h"

/**
 * @todo write docs
 */
class Message : public QHBoxLayout
{
    Q_OBJECT
public:
    Message(const Shared::Message& source, bool outgoing, const QString& sender, QWidget* parent = nullptr);
    ~Message();
    
    void setSender(const QString& sender);
    QString getId() const;
    QString getFileUrl() const;
    
    void addButton(const QIcon& icon, const QString& buttonText, const QString& comment = "");
    void showFile(const QString& path);
    void setProgress(qreal value, const QString& label = "");
    
signals:
    void downloadFile();
    
private:
    Shared::Message msg;
    QWidget* body;
    QVBoxLayout* bodyLayout;
    QLabel* date;
    QLabel* sender;
    QLabel* text;
    QGraphicsDropShadowEffect* shadow;
    QPushButton* downloadButton;
    QLabel* file;
    QProgressBar* progress;
    QLabel* fileComment;
    bool hasDownloadButton;
    bool hasProgress;
    bool hasFile;
    bool commentAdded;
  
private:
    void hideDownload();
    void hideProgress();
    void hideFile();
    void hideComment();
};

#endif // MESSAGE_H
