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
#include <QDebug>
#include <QScrollBar>
#include <QTimer>
#include <QGraphicsDropShadowEffect>
#include <QFileDialog>

Conversation::Conversation(bool muc, const QString& mJid, const QString mRes, const QString pJid, const QString pRes, const QString& acc, QWidget* parent):
    QWidget(parent),
    isMuc(muc),
    myJid(mJid),
    myResource(mRes),
    palJid(pJid),
    activePalResource(pRes),
    account(acc),
    line(new MessageLine(muc)),
    m_ui(new Ui::Conversation()),
    ker(),
    res(),
    vis(),
    thread(),
    statusIcon(0),
    statusLabel(0),
    scroll(down),
    manualSliderChange(false),
    requestingHistory(false),
    everShown(false)
{
    m_ui->setupUi(this);
    m_ui->splitter->setSizes({300, 0});
    m_ui->splitter->setStretchFactor(1, 0);
    
    statusIcon = m_ui->statusIcon;
    statusLabel = m_ui->statusLabel;
    
    connect(&ker, SIGNAL(enterPressed()), this, SLOT(onEnterPressed()));
    connect(&res, SIGNAL(resized()), this, SLOT(onScrollResize()));
    connect(&vis, SIGNAL(shown()), this, SLOT(onScrollResize()));
    connect(&vis, SIGNAL(hidden()), this, SLOT(onScrollResize()));
    connect(m_ui->sendButton, SIGNAL(clicked(bool)), this, SLOT(onEnterPressed()));
    connect(line, SIGNAL(resize(int)), this, SLOT(onMessagesResize(int)));
    connect(line, SIGNAL(downloadFile(const QString&, const QString&)), this, SIGNAL(downloadFile(const QString&, const QString&)));
    connect(line, SIGNAL(requestLocalFile(const QString&, const QString&)), this, SIGNAL(requestLocalFile(const QString&, const QString&)));
    //connect(m_ui->attachButton, SIGNAL(clicked(bool)), this, SLOT(onAttach()));
    
    m_ui->messageEditor->installEventFilter(&ker);
    
    QScrollBar* vs = m_ui->scrollArea->verticalScrollBar();
    m_ui->scrollArea->setWidget(line);
    vs->installEventFilter(&vis);
    vs->setBackgroundRole(QPalette::Base);
    vs->setAutoFillBackground(true);
    connect(vs, SIGNAL(valueChanged(int)), this, SLOT(onSliderValueChanged(int)));
    m_ui->scrollArea->installEventFilter(&res);
    
    applyVisualEffects();
}

Conversation::~Conversation()
{
}

void Conversation::applyVisualEffects()
{
    QGraphicsDropShadowEffect *e1 = new QGraphicsDropShadowEffect;
    e1->setBlurRadius(10);
    e1->setXOffset(0);
    e1->setYOffset(-2);
    e1->setColor(Qt::black);
    m_ui->bl->setGraphicsEffect(e1);
    
    QGraphicsDropShadowEffect *e2 = new QGraphicsDropShadowEffect;
    e2->setBlurRadius(7);
    e2->setXOffset(0);
    e2->setYOffset(2);
    e2->setColor(Qt::black);
    m_ui->ul->setGraphicsEffect(e2);
    
    QGraphicsDropShadowEffect *e3 = new QGraphicsDropShadowEffect;
    e3->setBlurRadius(10);
    e3->setXOffset(0);
    e3->setYOffset(2);
    e3->setColor(Qt::black);
    m_ui->ut->setGraphicsEffect(e3);
}

void Conversation::setName(const QString& name)
{
    m_ui->nameLabel->setText(name);
    setWindowTitle(name);
}

QString Conversation::getAccount() const
{
    return account;
}

QString Conversation::getJid() const
{
    return palJid;
}

void Conversation::addMessage(const Shared::Message& data)
{
    int pos = m_ui->scrollArea->verticalScrollBar()->sliderPosition();
    int max = m_ui->scrollArea->verticalScrollBar()->maximum();
    
    MessageLine::Position place = line->message(data);
    if (place == MessageLine::invalid) {
        return;
    }
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
        handleSendMessage(body);
    }
}

void Conversation::onMessagesResize(int amount)
{
    manualSliderChange = true;
    switch (scroll) {
        case down:
            m_ui->scrollArea->verticalScrollBar()->setValue(m_ui->scrollArea->verticalScrollBar()->maximum());
            break;
        case keep: {
            int max = m_ui->scrollArea->verticalScrollBar()->maximum();
            int value = m_ui->scrollArea->verticalScrollBar()->value() + amount;
            m_ui->scrollArea->verticalScrollBar()->setValue(value);
            
            if (value == max) {
                scroll = down;
            } else {
                scroll = nothing;
            }
        }
            break;
        default:
            break;
    }
    manualSliderChange = false;
}

void Conversation::onSliderValueChanged(int value)
{
    if (!manualSliderChange) {
        if (value == m_ui->scrollArea->verticalScrollBar()->maximum()) {
            scroll = down;
        } else {
            if (!requestingHistory && value == 0) {
                requestingHistory = true;
                line->showBusyIndicator();
                emit requestArchive(line->firstMessageId());
                scroll = keep;
            } else {
                scroll = nothing;
            }
        }
    }
}

void Conversation::responseArchive(const std::list<Shared::Message> list)
{
    requestingHistory = false;
    scroll = keep;
    
    line->hideBusyIndicator();
    for (std::list<Shared::Message>::const_iterator itr = list.begin(), end = list.end(); itr != end; ++itr) {
        addMessage(*itr);
    }
}

void Conversation::showEvent(QShowEvent* event)
{
    if (!everShown) {
        everShown = true;
        line->showBusyIndicator();
        requestingHistory = true;
        scroll = keep;
        emit requestArchive(line->firstMessageId());
    }
    emit shown();
    
    QWidget::showEvent(event);
    
}

void Conversation::onAttach()
{
    QFileDialog* d = new QFileDialog(this, "Chose a file to send");
    d->setFileMode(QFileDialog::ExistingFile);
    
    connect(d, SIGNAL(accepted()), this, SLOT(onFileSelected()));
    connect(d, SIGNAL(rejected()), d, SLOT(deleteLater()));
    
    d->show();
}

void Conversation::onFileSelected()
{
    QFileDialog* d = static_cast<QFileDialog*>(sender());
    
    qDebug() << d->selectedFiles();
    
    d->deleteLater();
}

void Conversation::setStatus(const QString& status)
{
    statusLabel->setText(status);
}

void Conversation::onScrollResize()
{
    if (everShown) {
        int size = m_ui->scrollArea->width();
        QScrollBar* bar = m_ui->scrollArea->verticalScrollBar();
        if (bar->isVisible()) {
            size -= bar->width();
        }
        line->setMaximumWidth(size);
    }
}

void Conversation::responseDownloadProgress(const QString& messageId, qreal progress)
{
    line->responseDownloadProgress(messageId, progress);
}

void Conversation::downloadError(const QString& messageId, const QString& error)
{
    line->downloadError(messageId, error);
}

void Conversation::responseLocalFile(const QString& messageId, const QString& path)
{
    line->responseLocalFile(messageId, path);
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

