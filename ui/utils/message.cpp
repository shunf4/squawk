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

Message::Message(const Shared::Message& source, bool p_outgoing, const QString& p_sender, const QString& avatarPath, QWidget* parent):
    QWidget(parent),
    outgoing(p_outgoing),
    msg(source),
    body(new QWidget()),
    statusBar(new QWidget()),
    bodyLayout(new QVBoxLayout(body)),
    layout(new QHBoxLayout(this)),
    date(new QLabel(msg.getTime().toLocalTime().toString())),
    sender(new QLabel(p_sender)),
    text(new QLabel()),
    shadow(new QGraphicsDropShadowEffect()),
    button(0),
    file(0),
    progress(0),
    fileComment(new QLabel()),
    statusIcon(0),
    editedLabel(0),
    avatar(new Image(avatarPath.size() == 0 ? Shared::iconPath("user", true) : avatarPath, 60)),
    hasButton(false),
    hasProgress(false),
    hasFile(false),
    commentAdded(false),
    hasStatusIcon(false),
    hasEditedLabel(false)
{
    setContentsMargins(0, 0, 0, 0);
    layout->setContentsMargins(10, 5, 10, 5);
    body->setBackgroundRole(QPalette::AlternateBase);
    body->setAutoFillBackground(true);
    
    QString bd = msg.getBody();
    //bd.replace(imgReg, "<img src=\"\\1\"/>");
    bd.replace(urlReg, "<a href=\"\\1\">\\1</a>");
    //bd.replace("\n", "<br>");
    text->setTextFormat(Qt::RichText);
    text->setText("<p style=\"white-space: pre-wrap;\">" + bd + "</p>");;
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
    
    shadow->setBlurRadius(10);
    shadow->setXOffset(1);
    shadow->setYOffset(1);
    shadow->setColor(Qt::black);
    body->setGraphicsEffect(shadow);
    avatar->setMaximumHeight(60);
    avatar->setMaximumWidth(60);
    
    statusBar->setContentsMargins(0, 0, 0, 0);
    QHBoxLayout* statusLay = new QHBoxLayout();
    statusLay->setContentsMargins(0, 0, 0, 0);
    statusBar->setLayout(statusLay);
    
    if (outgoing) {
        sender->setAlignment(Qt::AlignRight);
        date->setAlignment(Qt::AlignRight);
        statusIcon = new QLabel();
        setState();
        statusLay->addWidget(statusIcon);
        statusLay->addWidget(date);
        layout->addStretch();
        layout->addWidget(body);
        layout->addWidget(avatar);
        hasStatusIcon = true;
    } else {
        layout->addWidget(avatar);
        layout->addWidget(body);
        layout->addStretch();
        statusLay->addWidget(date);
    }
    
    bodyLayout->addWidget(statusBar);
    layout->setAlignment(avatar, Qt::AlignTop);
}

Message::~Message()
{
    if (!commentAdded) {
        delete fileComment;
    }
    //delete body;  //not sure if I should delete it here, it's probably already owned by the infrastructure and gonna die with the rest of the widget
    //delete avatar;
}

QString Message::getId() const
{
    return msg.getId();
}

QString Message::getSenderJid() const
{
    return msg.getFromJid();
}

QString Message::getSenderResource() const
{
    return msg.getFromResource();
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

bool Message::change(const QMap<QString, QVariant>& data)
{
    bool idChanged = msg.change(data);
    
    QString bd = msg.getBody();
    //bd.replace(imgReg, "<img src=\"\\1\"/>");
    bd.replace(urlReg, "<a href=\"\\1\">\\1</a>");
    text->setText(bd);
    if (bd.size() > 0) {
        text->show();
    } else {
        text->hide();
    }
    if (msg.getEdited()) {
        if (!hasEditedLabel) {
            editedLabel = new QLabel();
            QFont eFont = editedLabel->font();
            eFont.setItalic(true);
            eFont.setPointSize(eFont.pointSize() - 2);
            editedLabel->setFont(eFont);
            hasEditedLabel = true;
            QHBoxLayout* statusLay = static_cast<QHBoxLayout*>(statusBar->layout());
            if (hasStatusIcon) {
                statusLay->insertWidget(1, editedLabel);
            } else {
                statusLay->insertWidget(0, editedLabel);
            }
        }
    }
    if (hasStatusIcon) {
        setState();
    }
    
    
    return idChanged;
}

void Message::setState()
{
    Shared::Message::State state = msg.getState();
    QIcon q(Shared::icon(Shared::messageStateThemeIcons[static_cast<uint8_t>(state)]));
    QString tt = QCoreApplication::translate("Global", Shared::messageStateNames[static_cast<uint8_t>(state)].toLatin1());
    if (state == Shared::Message::State::error) {
        QString errText = msg.getErrorText();
        if (errText.size() > 0) {
            tt += ": " + errText;
        }
    }
    statusIcon->setToolTip(tt);
    statusIcon->setPixmap(q.pixmap(12, 12));
}

