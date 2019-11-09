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
#include <QMimeDatabase>
#include <QPixmap>
#include <QFileInfo>
#include "message.h"

const QRegExp urlReg("(?!<img\\ssrc=\")((?:https?|ftp)://\\S+)");
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
    fileComment(new QLabel()),
    errorText(""),
    hasDownloadButton(false),
    hasProgress(false),
    hasFile(false),
    commentAdded(false),
    errorDownloadingFile(false)
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
    if (!commentAdded) {
        delete fileComment;
    }
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
    hideFile();
    hideProgress();
    if (!hasDownloadButton) {
        hideComment();
        if (msg.getBody() == msg.getOutOfBandUrl()) {
            text->setText("");
            text->hide();
        }
        downloadButton = new QPushButton(QIcon::fromTheme("download"), tr("Download"));
        downloadButton->setToolTip("<a href=\"" + msg.getOutOfBandUrl() + "\">" + msg.getOutOfBandUrl() + "</a>");
        if (errorDownloadingFile) {
            fileComment->setWordWrap(true);
            fileComment->setText(tr("Error downloading file: %1\nYou can try again").arg(QCoreApplication::translate("NetworkErrors", errorText.toLatin1())));
        } else {
            fileComment->setText(tr("%1 is offering you to download a file").arg(sender->text()));
        }
        fileComment->show();
        connect(downloadButton, &QPushButton::clicked, this, &Message::onDownload);
        bodyLayout->insertWidget(2, fileComment);
        bodyLayout->insertWidget(3, downloadButton);
        hasDownloadButton = true;
        commentAdded = true;
    }
}

void Message::onDownload()
{
    emit downloadFile(msg.getId(), msg.getOutOfBandUrl());
}

void Message::setProgress(qreal value)
{
    hideFile();
    hideDownload();
    if (!hasProgress) {
        hideComment();
        if (msg.getBody() == msg.getOutOfBandUrl()) {
            text->setText("");
            text->hide();
        }
        progress = new QProgressBar();
        progress->setRange(0, 100);
        fileComment->setText("Downloading...");
        fileComment->show();
        bodyLayout->insertWidget(2, progress);
        bodyLayout->insertWidget(3, fileComment);
        hasProgress = true;
        commentAdded = true;
    }
    progress->setValue(value * 100);
}

void Message::showFile(const QString& path)
{
    hideDownload();
    hideProgress();
    if (!hasFile) {
        hideComment();
        if (msg.getBody() == msg.getOutOfBandUrl()) {
            text->setText("");
            text->hide();
        }
        QMimeDatabase db;
        QMimeType type = db.mimeTypeForFile(path);
        QStringList parts = type.name().split("/");
        QString big = parts.front();
        QFileInfo info(path);
        fileComment = new QLabel();
        if (big == "image") {
            file = new Image(path);
        } else {
            file = new QLabel();
            file->setPixmap(QIcon::fromTheme(type.iconName()).pixmap(50));
            file->setAlignment(Qt::AlignCenter);
            fileComment->setText(info.fileName());
            fileComment->setWordWrap(true);
            fileComment->show();
        }
        file->setContextMenuPolicy(Qt::ActionsContextMenu);
        QAction* openAction = new QAction(QIcon::fromTheme("document-new-from-template"), tr("Open"), file);
        connect(openAction, &QAction::triggered, [path]() {             //TODO need to get rid of this shame
            QDesktopServices::openUrl(QUrl::fromLocalFile(path));
        });
        file->addAction(openAction);
        bodyLayout->insertWidget(2, file);
        bodyLayout->insertWidget(3, fileComment);
        hasFile = true;
        commentAdded = true;
    }
}

void Message::hideComment()
{
    if (commentAdded) {
        bodyLayout->removeWidget(fileComment);
        fileComment->hide();
        fileComment->setWordWrap(false);
    }
}

void Message::hideDownload()
{
    if (hasDownloadButton) {
        downloadButton->deleteLater();
        downloadButton = 0;
        hasDownloadButton = false;
        errorDownloadingFile = false;
    }
}

void Message::hideFile()
{
    if (hasFile) {
        file->deleteLater();
        file = 0;
        hasFile = false;
    }
}

void Message::hideProgress()
{
    if (hasProgress) {
        progress->deleteLater();
        progress = 0;
        hasProgress = false;;
    }
}

void Message::showError(const QString& error)
{
    errorDownloadingFile = true;
    errorText = error;
    addDownloadDialog();
}
