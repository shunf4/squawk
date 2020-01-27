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
#include <QRegularExpression>
#include "message.h"

const QRegularExpression urlReg("(?<!<a\\shref=['\"])(?<!<img\\ssrc=['\"])("
                                "(?:https?|ftp):\\/\\/"
                                    "\\w+"
                                    "(?:"
                                        "[\\w\\.\\/\\:\\;\\?\\&\\=\\@\\%\\#\\+\\-]?"
                                        "(?:"
                                            "\\([\\w\\.\\/\\:\\;\\?\\&\\=\\@\\%\\#\\+\\-]+\\)"
                                        ")?"
                                    ")*"
                                ")");
const QRegularExpression imgReg("((?:https?|ftp)://\\S+\\.(?:jpg|jpeg|png|svg|gif))");

Message::Message(const Shared::Message& source, bool outgoing, const QString& p_sender, const QString& avatarPath, QWidget* parent):
    QHBoxLayout(parent),
    msg(source),
    body(new QWidget()),
    bodyLayout(new QVBoxLayout(body)),
    date(new QLabel(msg.getTime().toLocalTime().toString())),
    sender(new QLabel(p_sender)),
    text(new QLabel()),
    shadow(new QGraphicsDropShadowEffect()),
    button(0),
    file(0),
    progress(0),
    fileComment(new QLabel()),
    avatar(new Image(avatarPath.size() == 0 ? Shared::iconPath("user", true) : avatarPath, 60)),
    hasButton(false),
    hasProgress(false),
    hasFile(false),
    commentAdded(false)
{
    body->setBackgroundRole(QPalette::AlternateBase);
    body->setAutoFillBackground(true);
    
    QString bd = msg.getBody();
    //bd.replace(imgReg, "<img src=\"\\1\"/>");
    bd.replace(urlReg, "<a href=\"\\1\">\\1</a>");
    bd.replace("\n", "<br>");
    text->setTextFormat(Qt::RichText);
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
    avatar->setMaximumHeight(60);
    avatar->setMaximumWidth(60);
    QVBoxLayout* aLay = new QVBoxLayout();
    aLay->addWidget(avatar);
    aLay->addStretch();
    
    
    if (outgoing) {
        sender->setAlignment(Qt::AlignRight);
        date->setAlignment(Qt::AlignRight);
        addStretch();
        addWidget(body);
        addItem(aLay);
    } else {
        addItem(aLay);
        addWidget(body);
        addStretch();
    }
}

Message::~Message()
{
    if (!commentAdded) {
        delete fileComment;
    }
    delete body;
    delete avatar;
}

QString Message::getId() const
{
    return msg.getId();
}

QString Message::getFileUrl() const
{
    return msg.getOutOfBandUrl();
}

void Message::setSender(const QString& p_sender)
{
    sender->setText(p_sender);
}

void Message::addButton(const QIcon& icon, const QString& buttonText, const QString& tooltip)
{
    hideFile();
    hideProgress();
    if (!hasButton) {
        hideComment();
        if (msg.getBody() == msg.getOutOfBandUrl()) {
            text->setText("");
            text->hide();
        }
        button = new QPushButton(icon, buttonText);
        button->setToolTip(tooltip);
        connect(button, &QPushButton::clicked, this, &Message::buttonClicked);
        bodyLayout->insertWidget(2, button);
        hasButton = true;
    }
}

void Message::setProgress(qreal value)
{
    hideFile();
    hideButton();
    if (!hasProgress) {
        hideComment();
        if (msg.getBody() == msg.getOutOfBandUrl()) {
            text->setText("");
            text->hide();
        }
        progress = new QProgressBar();
        progress->setRange(0, 100);
        bodyLayout->insertWidget(2, progress);
        hasProgress = true;
    }
    progress->setValue(value * 100);
}

void Message::showFile(const QString& path)
{
    hideButton();
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
        if (big == "image") {
            file = new Image(path);
        } else {
            file = new QLabel();
            file->setPixmap(QIcon::fromTheme(type.iconName()).pixmap(50));
            file->setAlignment(Qt::AlignCenter);
            showComment(info.fileName(), true);
        }
        file->setContextMenuPolicy(Qt::ActionsContextMenu);
        QAction* openAction = new QAction(QIcon::fromTheme("document-new-from-template"), tr("Open"), file);
        connect(openAction, &QAction::triggered, [path]() {             //TODO need to get rid of this shame
            QDesktopServices::openUrl(QUrl::fromLocalFile(path));
        });
        file->addAction(openAction);
        bodyLayout->insertWidget(2, file);
        hasFile = true;
    }
}

void Message::hideComment()
{
    if (commentAdded) {
        bodyLayout->removeWidget(fileComment);
        fileComment->hide();
        fileComment->setWordWrap(false);
        commentAdded = false;
    }
}

void Message::hideButton()
{
    if (hasButton) {
        button->deleteLater();
        button = 0;
        hasButton = false;
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
void Message::showComment(const QString& comment, bool wordWrap)
{
    if (!commentAdded) {
        int index = 2;
        if (hasFile) {
            index++;
        }
        if (hasButton) {
            index++;
        }
        if (hasProgress) {
            index++;
        }
        bodyLayout->insertWidget(index, fileComment);
        fileComment->show();
        commentAdded = true;
    }
    fileComment->setWordWrap(wordWrap);
    fileComment->setText(comment);
}

const Shared::Message & Message::getMessage() const
{
    return msg;
}

void Message::setAvatarPath(const QString& p_path)
{
    if (p_path.size() == 0) {
        avatar->setPath(Shared::iconPath("user", true));
    } else {
        avatar->setPath(p_path);
    }
}
