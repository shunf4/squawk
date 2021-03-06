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
#include "messagetextedit.h"
#include "ui_conversation.h"

#include <QDebug>
#include <QClipboard>
#include <QScrollBar>
#include <QTimer>
#include <QFileDialog>
#include <QMimeDatabase>
#include <unistd.h>
#include <QAbstractTextDocumentLayout>
#include <QCoreApplication>
#include <QTemporaryFile>
#include <QDir>
#include <QMenu>
#include <QClipboard>

Conversation::Conversation(bool muc, Models::Account* acc, Models::Element* el, const QString pJid, const QString pRes, QWidget* parent):
    QWidget(parent),
    isMuc(muc),
    account(acc),
    element(el),
    palJid(pJid),
    activePalResource(pRes),
    m_ui(new Ui::Conversation()),
    ker(),
    thread(),
    statusIcon(0),
    statusLabel(0),
    filesLayout(0),
    overlay(new QWidget()),
    filesToAttach(),
    feed(new FeedView()),
    delegate(new MessageDelegate(this)),
    manualSliderChange(false),
    tsb(QApplication::style()->styleHint(QStyle::SH_ScrollBar_Transient) == 1),
    shadow(10, 1, Qt::black, this),
    contextMenu(new QMenu())
{
    m_ui->setupUi(this);
    
    shadow.setFrames(true, false, true, false);
    
    feed->setItemDelegate(delegate);
    feed->setFrameShape(QFrame::NoFrame);
    feed->setContextMenuPolicy(Qt::CustomContextMenu);
    delegate->initializeFonts(feed->getFont());
    feed->setModel(el->feed);
    el->feed->incrementObservers();
    m_ui->widget->layout()->addWidget(feed);
    
    connect(el->feed, &Models::MessageFeed::newMessage, this, &Conversation::onFeedMessage);
    connect(feed, &FeedView::resized, this, &Conversation::positionShadow);
    connect(feed, &FeedView::customContextMenuRequested, this, &Conversation::onFeedContext);
    
    connect(acc, &Models::Account::childChanged, this, &Conversation::onAccountChanged);
    
    filesLayout = new FlowLayout(m_ui->filesPanel, 0);
    m_ui->filesPanel->setLayout(filesLayout);
    
    statusIcon = m_ui->statusIcon;
    statusLabel = m_ui->statusLabel;
    
    connect(&ker, &KeyEnterReceiver::enterPressed, this, &Conversation::onEnterPressed);
    connect(m_ui->messageEditor, &MessageTextEdit::imageInserted, this, &Conversation::onImageInserted);
    connect(m_ui->messageEditor, &MessageTextEdit::fileInserted, this, &Conversation::onFileInserted);
    connect(m_ui->sendButton, &QPushButton::clicked, this, &Conversation::onEnterPressed);
    connect(m_ui->attachButton, &QPushButton::clicked, this, &Conversation::onAttach);
    connect(m_ui->clearButton, &QPushButton::clicked, this, &Conversation::onClearButton);
    connect(m_ui->messageEditor->document()->documentLayout(), &QAbstractTextDocumentLayout::documentSizeChanged, 
            this, &Conversation::onTextEditDocSizeChanged);
    
    m_ui->messageEditor->installEventFilter(&ker);



    //line->setAutoFillBackground(false);
    //if (testAttribute(Qt::WA_TranslucentBackground)) {
        //m_ui->scrollArea->setAutoFillBackground(false);
    //} else {
        //m_ui->scrollArea->setBackgroundRole(QPalette::Base);
    //}
    
    //line->setMyAvatarPath(acc->getAvatarPath());
    //line->setMyName(acc->getName());
    
    initializeOverlay();
}

Conversation::~Conversation()
{
    delete contextMenu;
    
    element->feed->decrementObservers();
}

void Conversation::onAccountChanged(Models::Item* item, int row, int col)
{
    if (item == account) {
        if (col == 2 && account->getState() == Shared::ConnectionState::connected) {        //to request the history when we're back online after reconnect
            //if (!requestingHistory) {
                //requestingHistory = true;
                //line->showBusyIndicator();
                //emit requestArchive("");
                //scroll = down;
            //}
        }
    }
}

void Conversation::initializeOverlay()
{
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
        emit sendMessage(msg);
    }
    if (filesToAttach.size() > 0) {
        for (Badge* badge : filesToAttach) {
            Shared::Message msg = createMessage();
            msg.setAttachPath(badge->id);
            element->feed->registerUpload(msg.getId());
            emit sendMessage(msg);
        }
         clearAttachedFiles();
    }
}

void Conversation::onImageInserted(const QImage& image)
{
    if (image.isNull()) {
        return;
    }
    QTemporaryFile *tempFile = new QTemporaryFile(QDir::tempPath() + QStringLiteral("/squawk_img_attach_XXXXXX.png"), QApplication::instance());
    tempFile->open();
    image.save(tempFile, "PNG");
    tempFile->close();
    qDebug() << "image on paste temp file: " << tempFile->fileName();
    addAttachedFile(tempFile->fileName());

    // The file, if successfully uploaded, will be copied to Download folder.
    // On application closing, this temporary file will be automatically removed by Qt.
    // See Core::NetworkAccess::onUploadFinished.
}

void Conversation::onFileInserted(const QString& localPath)
{
    addAttachedFile(localPath);
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

Models::Roster::ElId Conversation::getId() const
{
    return {getAccount(), getJid()};
}

void Conversation::addAttachedFile(const QString& path)
{
    QMimeDatabase db;
    QMimeType type = db.mimeTypeForFile(path);
    QFileInfo info(path);

    QIcon fileIcon = QIcon::fromTheme(type.iconName());
    if (fileIcon.isNull()) {
        fileIcon.addFile(QString::fromUtf8(":/images/fallback/dark/big/mail-attachment.svg"), QSize(), QIcon::Normal, QIcon::Off);
    }
    Badge* badge = new Badge(path, info.fileName(), fileIcon);
    
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
    shadow.setFrames(top, right, bottom, left);
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

void Conversation::onFeedMessage(const Shared::Message& msg)
{
    this->onMessage(msg);
}

void Conversation::onMessage(const Shared::Message& msg)
{
    if (!msg.getForwarded()) {
        QApplication::alert(this);
        if (window()->windowState().testFlag(Qt::WindowMinimized)) {
            emit notifyableMessage(getAccount(), msg);
        }
    }
}

void Conversation::positionShadow()
{
    int w = width();
    int h = feed->height();
    
    shadow.resize(w, h);
    shadow.move(feed->pos());
    shadow.raise();
}

void Conversation::onFeedContext(const QPoint& pos)
{
    QModelIndex index = feed->indexAt(pos);
    if (index.isValid()) {
        Shared::Message* item = static_cast<Shared::Message*>(index.internalPointer());
        
        contextMenu->clear();
        bool showMenu = false;
        if (item->getState() == Shared::Message::State::error) {
            showMenu = true;
            QString id = item->getId();
            QAction* resend = contextMenu->addAction(Shared::icon("view-refresh"), tr("Try sending again")); 
            connect(resend, &QAction::triggered, [this, id]() {
                element->feed->registerUpload(id);
                emit resendMessage(id);
            });
        }
        
        QString path = item->getAttachPath();
        QString remotePath = item->getOutOfBandUrl();
        if (path.size() > 0) {
            showMenu = true;
            QAction* open = contextMenu->addAction(Shared::icon("document-preview"), tr("Open")); 
            connect(open, &QAction::triggered, [path]() {
                QDesktopServices::openUrl(QUrl::fromLocalFile(path));
            });
            
            QAction* show = contextMenu->addAction(Shared::icon("folder"), tr("Show in folder")); 
            connect(show, &QAction::triggered, [path]() {
                Shared::Global::highlightInFileManager(path);
            });

            QAction* copyLink = contextMenu->addAction(Shared::icon("copy"), tr("Copy link address"));
            connect(copyLink, &QAction::triggered, [remotePath]() {
                QApplication::clipboard()->setText(remotePath);
            });
        }
        
        if (showMenu) {
            contextMenu->popup(feed->viewport()->mapToGlobal(pos));
        }
    }
}
