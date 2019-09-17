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

#include <QDebug>
#include "message.h"

const QRegExp urlReg("^(?!<img\\ssrc=\")((?:https?|ftp)://\\S+)");
const QRegExp imgReg("((?:https?|ftp)://\\S+\\.(?:jpg|jpeg|png|svg|gif))");

Message::Message(const Shared::Message& source, bool outgoing, const QString& p_sender, QWidget* parent):
    QHBoxLayout(parent),
    msg(source),
    body(new QWidget()),
    bodyLayout(new QVBoxLayout(body)),
    date(new QLabel(msg.getTime().toLocalTime().toString())),
    sender(new QLabel(p_sender)),
    text(new QLabel()),
    shadow(new QGraphicsDropShadowEffect()),
    downloadButton(0),
    file(0),
    progress(0),
    fileComment(0),
    hasDownloadButton(false),
    hasProgress(false),
    hasFile(false)
{
    body->setBackgroundRole(QPalette::AlternateBase);
    body->setAutoFillBackground(true);
    
    QString bd = msg.getBody();
    //bd.replace(imgReg, "<img src=\"\\1\"/>");
    bd.replace(urlReg, "<a href=\"\\1\">\\1</a>");
    text->setText(bd);;
    text->setTextInteractionFlags(text->textInteractionFlags() | Qt::TextSelectableByMouse | Qt::LinksAccessibleByMouse);
    text->setWordWrap(true);
    text->setOpenExternalLinks(true);
    if (bd.size() == 0) {
        text->hide();
    }
    
    QFont dFont = date->font();
    dFont.setItalic(true);
    dFont.setPointSize(dFont.pointSize() - 2);
    date->setFont(dFont);
    date->setForegroundRole(QPalette::ToolTipText);
    
    QFont f;
    f.setBold(true);
    sender->setFont(f);
    
    bodyLayout->addWidget(sender);
    bodyLayout->addWidget(text);
    bodyLayout->addWidget(date);
    
    shadow->setBlurRadius(10);
    shadow->setXOffset(1);
    shadow->setYOffset(1);
    shadow->setColor(Qt::black);
    body->setGraphicsEffect(shadow);
    
    if (outgoing) {
        addWidget(body);
        addStretch();
    } else {
        sender->setAlignment(Qt::AlignRight);
        date->setAlignment(Qt::AlignRight);
        addStretch();
        addWidget(body);
    }
}

Message::~Message()
{
}

QString Message::getId() const
{
    return msg.getId();
}

void Message::setSender(const QString& p_sender)
{
    sender->setText(p_sender);
}

void Message::addDownloadDialog()
{
    if (hasFile) {
        file->deleteLater();
        file = 0;
        hasFile = false;
    }
    if (hasProgress) {
        progress->deleteLater();
        progress = 0;
        hasProgress = false;;
    }
    if (!hasDownloadButton) {
        if (msg.getBody() == msg.getOutOfBandUrl()) {
            text->setText("");
            text->hide();
        }
        downloadButton = new QPushButton(QIcon::fromTheme("download"), "Download");
        fileComment = new QLabel(sender->text() + " is offering you to download a file");
        connect(downloadButton, SIGNAL(clicked()), this, SLOT(onDownload()));
        bodyLayout->insertWidget(2, fileComment);
        bodyLayout->insertWidget(3, downloadButton);
        hasDownloadButton = true;
    }
}

void Message::onDownload()
{
    emit downloadFile(msg.getId(), msg.getOutOfBandUrl());
}

void Message::setProgress(qreal value)
{
    if (hasFile) {
        file->deleteLater();
        file = 0;
        hasFile = false;
    }
    if (hasDownloadButton) {
        downloadButton->deleteLater();
        fileComment->deleteLater();
        downloadButton = 0;
        fileComment = 0;
        hasDownloadButton = false;
    }
    if (!hasProgress) {
        if (msg.getBody() == msg.getOutOfBandUrl()) {
            text->setText("");
            text->hide();
        }
        progress = new QLabel(std::to_string(value).c_str());
        bodyLayout->insertWidget(2, progress);
        hasProgress = true;
    }
}

void Message::showFile(const QString& path)
{
    if (hasDownloadButton) {
        downloadButton->deleteLater();
        fileComment->deleteLater();
        downloadButton = 0;
        fileComment = 0;
        hasDownloadButton = false;
    }
    if (hasProgress) {
        progress->deleteLater();
        progress = 0;
        hasProgress = false;
    }
    if (!hasFile) {
        if (msg.getBody() == msg.getOutOfBandUrl()) {
            text->setText("");
            text->hide();
        }
        //file = new QLabel("<img src=\"" + path + "\">");
        file = new QLabel();
        file->setPixmap(QPixmap(path));
        //file->setScaledContents(true);
        bodyLayout->insertWidget(2, file);
        hasFile = true;
    }
}
