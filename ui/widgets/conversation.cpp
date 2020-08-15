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

#include "conversation.h"
#include "ui_conversation.h"
#include "ui/utils/dropshadoweffect.h"

#include <QDebug>
#include <QScrollBar>
#include <QTimer>
#include <QFileDialog>
#include <QMimeDatabase>
#include <unistd.h>
#include <QAbstractTextDocumentLayout>
#include <QCoreApplication>

Conversation::Conversation(bool muc, Models::Account* acc, Models::Element* el, const QString pJid, const QString pRes, QWidget* parent):
    QWidget(parent),
    isMuc(muc),
    account(acc),
    palJid(pJid),
    activePalResource(pRes),
    m_ui(new Ui::Conversation()),
    ker(),
    scrollResizeCatcher(),
    vis(),
    thread(),
    statusIcon(0),
    statusLabel(0),
    filesLayout(0),
    overlay(new QWidget()),
    filesToAttach(),
    feed(new FeedView()),
    scroll(down),
    manualSliderChange(false),
    requestingHistory(false),
    everShown(false),
    tsb(QApplication::style()->styleHint(QStyle::SH_ScrollBar_Transient) == 1)
{
    m_ui->setupUi(this);
    
    feed->setModel(el->feed);
    m_ui->widget->layout()->addWidget(feed);
    
    connect(acc, &Models::Account::childChanged, this, &Conversation::onAccountChanged);
    
    filesLayout = new FlowLayout(m_ui->filesPanel, 0);
    m_ui->filesPanel->setLayout(filesLayout);
    
    statusIcon = m_ui->statusIcon;
    statusLabel = m_ui->statusLabel;
    
    connect(&ker, &KeyEnterReceiver::enterPressed, this, &Conversation::onEnterPressed);
    connect(&scrollResizeCatcher, &Resizer::resized, this, &Conversation::onScrollResize);
    connect(&vis, &VisibilityCatcher::shown, this, &Conversation::onScrollResize);
    connect(&vis, &VisibilityCatcher::hidden, this, &Conversation::onScrollResize);
    connect(m_ui->sendButton, &QPushButton::clicked, this, &Conversation::onEnterPressed);
    //connect(line, &MessageLine::resize, this, &Conversation::onMessagesResize);
    //connect(line, &MessageLine::downloadFile, this, &Conversation::downloadFile);
    //connect(line, &MessageLine::uploadFile, this, qOverload<const Shared::Message&, const QString&>(&Conversation::sendMessage));
    //connect(line, &MessageLine::requestLocalFile, this, &Conversation::requestLocalFile);
    connect(m_ui->attachButton, &QPushButton::clicked, this, &Conversation::onAttach);
    connect(m_ui->clearButton, &QPushButton::clicked, this, &Conversation::onClearButton);
    connect(m_ui->messageEditor->document()->documentLayout(), &QAbstractTextDocumentLayout::documentSizeChanged, 
            this, &Conversation::onTextEditDocSizeChanged);
    
    m_ui->messageEditor->installEventFilter(&ker);
    
    //QScrollBar* vs = m_ui->scrollArea->verticalScrollBar();
    //m_ui->scrollArea->setWidget(line);
    //vs->installEventFilter(&vis);
    
    //line->setAutoFillBackground(false);
    //if (testAttribute(Qt::WA_TranslucentBackground)) {
        //m_ui->scrollArea->setAutoFillBackground(false);
    //} else {
        //m_ui->scrollArea->setBackgroundRole(QPalette::Base);
    //}
    
    //connect(vs, &QScrollBar::valueChanged, this, &Conversation::onSliderValueChanged);
    //m_ui->scrollArea->installEventFilter(&scrollResizeCatcher);
    
    //line->setMyAvatarPath(acc->getAvatarPath());
    //line->setMyName(acc->getName());
    
    QGridLayout* gr = static_cast<QGridLayout*>(layout());
    QLabel* progressLabel = new QLabel(tr("Drop files here to attach them to your message"));
    gr->addWidget(overlay, 0, 0, 2, 1);
    QVBoxLayout* nl = new QVBoxLayout();
    QGraphicsOpacityEffect* opacity = new QGraphicsOpacityEffect();
    opacity->setOpacity(0.8);
    overlay->setLayout(nl);
    overlay->setBackgroundRole(QPalette::Base);
    overlay->setAutoFillBackground(true);
    overlay->setGraphicsEffect(opacity);
    progressLabel->setAlignment(Qt::AlignCenter);
    QFont pf = progressLabel->font();
    pf.setBold(true);
    pf.setPointSize(26);
    progressLabel->setWordWrap(true);
    progressLabel->setFont(pf);
    nl->addStretch();
    nl->addWidget(progressLabel);
    nl->addStretch();
    overlay->hide();
    
    applyVisualEffects();
}

Conversation::~Conversation()
{
}

void Conversation::onAccountChanged(Models::Item* item, int row, int col)
{
    if (item == account) {
        if (col == 2 && account->getState() == Shared::ConnectionState::connected) {
            if (!requestingHistory) {
                requestingHistory = true;
                //line->showBusyIndicator();
                //emit requestArchive("");
                //scroll = down;
            }
        }
    }
}

void Conversation::applyVisualEffects()
{
//     DropShadowEffect *e1 = new DropShadowEffect;
//     e1->setBlurRadius(10);
//     e1->setColor(Qt::black);
//     e1->setThickness(1);
//     e1->setFrame(true, false, true, false);
//     m_ui->scrollArea->setGraphicsEffect(e1);
}

void Conversation::setName(const QString& name)
{
    m_ui->nameLabel->setText(name);
    setWindowTitle(name);
}

QString Conversation::getAccount() const
{
    return account->getName();
}

QString Conversation::getJid() const
{
    return palJid;
}

void Conversation::changeMessage(const QString& id, const QMap<QString, QVariant>& data)
{
//     line->changeMessage(id, data);
}

KeyEnterReceiver::KeyEnterReceiver(QObject* parent): QObject(parent), ownEvent(false) {}

bool KeyEnterReceiver::eventFilter(QObject* obj, QEvent* event)
{
    QEvent::Type type = event->type();
    if (type == QEvent::KeyPress) {
        QKeyEvent* key = static_cast<QKeyEvent*>(event);
        int k = key->key();
        if (k == Qt::Key_Enter || k == Qt::Key_Return) {
            Qt::KeyboardModifiers mod = key->modifiers();
            if (mod & Qt::ControlModifier) {
                mod = mod & ~Qt::ControlModifier;
                QKeyEvent* nEvent = new QKeyEvent(event->type(), k, mod, key->text(), key->isAutoRepeat(), key->count());
                QCoreApplication::postEvent(obj, nEvent);
                ownEvent = true;
                return true;
            } else {
                if (ownEvent) {
                    ownEvent = false;
                } else {
                    emit enterPressed();
                    return true;
                }
            }
        }
    }
    return QObject::eventFilter(obj, event);
}

QString Conversation::getPalResource() const
{
    return activePalResource;
}

void Conversation::setPalResource(const QString& res)
{
    activePalResource = res;
}

void Conversation::onEnterPressed()
{
    QString body(m_ui->messageEditor->toPlainText());
    
    if (body.size() > 0) {
        m_ui->messageEditor->clear();
        Shared::Message msg = createMessage();
        msg.setBody(body);
        //addMessage(msg);
        emit sendMessage(msg);
    }
    if (filesToAttach.size() > 0) {
//         for (Badge* badge : filesToAttach) {
//             Shared::Message msg = createMessage();
//             line->appendMessageWithUpload(msg, badge->id);
//             usleep(1000);       //this is required for the messages not to have equal time when appending into messageline
//         }
//         clearAttachedFiles();
    }
}

void Conversation::appendMessageWithUpload(const Shared::Message& data, const QString& path)
{
//     line->appendMessageWithUploadNoSiganl(data, path);
}

void Conversation::onMessagesResize(int amount)
{
//     manualSliderChange = true;
//     switch (scroll) {
//         case down:
//             m_ui->scrollArea->verticalScrollBar()->setValue(m_ui->scrollArea->verticalScrollBar()->maximum());
//             break;
//         case keep: {
//             int max = m_ui->scrollArea->verticalScrollBar()->maximum();
//             int value = m_ui->scrollArea->verticalScrollBar()->value() + amount;
//             m_ui->scrollArea->verticalScrollBar()->setValue(value);
//             
//             if (value == max) {
//                 scroll = down;
//             } else {
//                 scroll = nothing;
//             }
//         }
//             break;
//         default:
//             break;
//     }
//     manualSliderChange = false;
}

void Conversation::onSliderValueChanged(int value)
{
//     if (!manualSliderChange) {
//         if (value == m_ui->scrollArea->verticalScrollBar()->maximum()) {
//             scroll = down;
//         } else {
//             if (!requestingHistory && value == 0) {
//                 requestingHistory = true;
//                  line->showBusyIndicator();
//                  emit requestArchive(line->firstMessageId());
//                 scroll = keep;
//             } else {
//                 scroll = nothing;
//             }
//         }
//     }
}

void Conversation::responseArchive(const std::list<Shared::Message> list)
{
//     requestingHistory = false;
//     scroll = keep;
//     
//     line->hideBusyIndicator();
//     for (std::list<Shared::Message>::const_iterator itr = list.begin(), end = list.end(); itr != end; ++itr) {
//         addMessage(*itr);
//     }
}

void Conversation::showEvent(QShowEvent* event)
{
    if (!everShown) {
        everShown = true;
//         line->showBusyIndicator();
        requestingHistory = true;
        scroll = keep;
        emit requestArchive("");
    }
    emit shown();
    
    QWidget::showEvent(event);
    
}

void Conversation::onAttach()
{
    QFileDialog* d = new QFileDialog(this, tr("Chose a file to send"));
    d->setFileMode(QFileDialog::ExistingFile);
    
    connect(d, &QFileDialog::accepted, this, &Conversation::onFileSelected);
    connect(d, &QFileDialog::rejected, d, &QFileDialog::deleteLater);
    
    d->show();
}

void Conversation::onFileSelected()
{
    QFileDialog* d = static_cast<QFileDialog*>(sender());
    
    for (const QString& path : d->selectedFiles()) {
        addAttachedFile(path);
    }
    
    d->deleteLater();
}

void Conversation::setStatus(const QString& status)
{
    statusLabel->setText(Shared::processMessageBody(status));
}

void Conversation::onScrollResize()
{
//     if (everShown) {
//         int size = m_ui->scrollArea->width();
//         QScrollBar* bar = m_ui->scrollArea->verticalScrollBar();
//         if (bar->isVisible() && !tsb) {
//             size -= bar->width();
//             
//         }
//          line->setMaximumWidth(size);
//     }
}

void Conversation::responseFileProgress(const QString& messageId, qreal progress)
{
//     line->fileProgress(messageId, progress);
}

void Conversation::fileError(const QString& messageId, const QString& error)
{
//     line->fileError(messageId, error);
}

void Conversation::responseLocalFile(const QString& messageId, const QString& path)
{
//     line->responseLocalFile(messageId, path);
}

Models::Roster::ElId Conversation::getId() const
{
    return {getAccount(), getJid()};
}

void Conversation::addAttachedFile(const QString& path)
{
    QMimeDatabase db;
    QMimeType type = db.mimeTypeForFile(path);
    QFileInfo info(path);
    
    Badge* badge = new Badge(path, info.fileName(), QIcon::fromTheme(type.iconName()));
    
    connect(badge, &Badge::close, this, &Conversation::onBadgeClose);
    try {
        filesToAttach.push_back(badge);
        filesLayout->addWidget(badge);
        if (filesLayout->count() == 1) {
            filesLayout->setContentsMargins(3, 3, 3, 3);
        }
    } catch (const W::Order<Badge*, Badge::Comparator>::Duplicates& e) {
        delete badge;
    } catch (...) {
        throw;
    }
}

void Conversation::removeAttachedFile(Badge* badge)
{
    W::Order<Badge*, Badge::Comparator>::const_iterator itr = filesToAttach.find(badge);
    if (itr != filesToAttach.end()) {
        filesToAttach.erase(badge);
        if (filesLayout->count() == 1) {
            filesLayout->setContentsMargins(0, 0, 0, 0);
        }
        badge->deleteLater();
    }
}

void Conversation::onBadgeClose()
{
    Badge* badge = static_cast<Badge*>(sender());
    removeAttachedFile(badge);
}

void Conversation::clearAttachedFiles()
{
    for (Badge* badge : filesToAttach) {
        badge->deleteLater();
    }
    filesToAttach.clear();
    filesLayout->setContentsMargins(0, 0, 0, 0);
}

void Conversation::onClearButton()
{
    clearAttachedFiles();
    m_ui->messageEditor->clear();
}

void Conversation::setAvatar(const QString& path)
{
    if (path.size() == 0) {
        m_ui->avatar->setPixmap(Shared::icon("user", true).pixmap(QSize(50, 50)));
    } else {
        m_ui->avatar->setPixmap(path);
    }
}

void Conversation::onTextEditDocSizeChanged(const QSizeF& size)
{
    m_ui->messageEditor->setMaximumHeight(int(size.height()));
}

void Conversation::setFeedFrames(bool top, bool right, bool bottom, bool left)
{
    //static_cast<DropShadowEffect*>(m_ui->scrollArea->graphicsEffect())->setFrame(top, right, bottom, left);
}

void Conversation::dragEnterEvent(QDragEnterEvent* event)
{
    bool accept = false;
    if (event->mimeData()->hasUrls()) {
        QList<QUrl> list = event->mimeData()->urls();
        for (const QUrl& url : list) {
            if (url.isLocalFile()) {
                QFileInfo info(url.toLocalFile());
                if (info.isReadable() && info.isFile()) {
                    accept = true;
                    break;
                }
            }
        }
    }
    if (accept) {
        event->acceptProposedAction();
        overlay->show();
    }
}

void Conversation::dragLeaveEvent(QDragLeaveEvent* event)
{
    overlay->hide();
}

void Conversation::dropEvent(QDropEvent* event)
{
    bool accept = false;
    if (event->mimeData()->hasUrls()) {
        QList<QUrl> list = event->mimeData()->urls();
        for (const QUrl& url : list) {
            if (url.isLocalFile()) {
                QFileInfo info(url.toLocalFile());
                if (info.isReadable() && info.isFile()) {
                    addAttachedFile(info.canonicalFilePath());
                    accept = true;
                }
            }
        }
    }
    if (accept) {
        event->acceptProposedAction();
    }
    overlay->hide();
}

Shared::Message Conversation::createMessage() const
{
    Shared::Message msg;
    msg.setOutgoing(true);
    msg.generateRandomId();
    msg.setCurrentTime();
    msg.setState(Shared::Message::State::pending);
    return msg;
}

bool VisibilityCatcher::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() == QEvent::Show) {
        emit shown();
    }
    
    if (event->type() == QEvent::Hide) {
        emit hidden();
    }
    
    return false;
}

VisibilityCatcher::VisibilityCatcher(QWidget* parent):
QObject(parent)
{
}

