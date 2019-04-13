/*
 * <one line to give the program's name and a brief idea of what it does.>
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

Conversation::Conversation(Models::Contact* p_contact, QWidget* parent):
    QWidget(parent),
    contact(p_contact),
    line(new MessageLine()),
    m_ui(new Ui::Conversation()),
    ker(),
    activePalResource(),
    thread(),
    scroll(nothing)
{
    m_ui->setupUi(this);
    m_ui->splitter->setSizes({300, 0});
    m_ui->splitter->setStretchFactor(1, 0);
    
    setName(p_contact->getName());
    setState(p_contact->getAvailability());
    
    connect(contact, SIGNAL(childChanged(Models::Item*, int, int)), this, SLOT(onContactChanged(Models::Item*, int, int)));
    connect(&ker, SIGNAL(enterPressed()), this, SLOT(onEnterPressed()));
    connect(m_ui->sendButton, SIGNAL(clicked(bool)), this, SLOT(onEnterPressed()));
    
    m_ui->messageEditor->installEventFilter(&ker);
    
    Models::Contact::Messages deque;
    contact->getMessages(deque);
    
    for (Models::Contact::Messages::const_iterator itr = deque.begin(), end = deque.end(); itr != end; ++itr) {
        addMessage(*itr);
    }
    
    line->setMyName(p_contact->getAccountName());
    connect(line, SIGNAL(resize(int)), this, SLOT(onMessagesResize(int)));
    
    m_ui->scrollArea->setWidget(line);
    m_ui->scrollArea->verticalScrollBar()->setBackgroundRole(QPalette::Base);
    m_ui->scrollArea->verticalScrollBar()->setAutoFillBackground(true);;
}

Conversation::~Conversation()
{
    disconnect(contact, SIGNAL(childChanged(Models::Item*, int, int)), this, SLOT(onContactChanged(Models::Item*, int, int)));
}

void Conversation::setName(const QString& name)
{
    if (name == "") {
        m_ui->nameLabel->setText(getJid());
    } else {
        m_ui->nameLabel->setText(name);
        line->setPalName(getJid(), name);
    }
}

void Conversation::setState(Shared::Availability state)
{
    m_ui->statusIcon->setPixmap(QIcon::fromTheme(Shared::availabilityThemeIcons[state]).pixmap(50));
    m_ui->statusIcon->setToolTip(Shared::availabilityNames[state]);
}

void Conversation::setStatus(const QString& status)
{
    m_ui->statusLabel->setText(status);
}

QString Conversation::getAccount() const
{
    return contact->getAccountName();
}

QString Conversation::getJid() const
{
    return contact->getJid();
}

void Conversation::onContactChanged(Models::Item* item, int row, int col)
{
    if (item == contact) {
        switch (col) {
            case 0:
                setName(contact->getName());
                break;
            case 3:
                setState(contact->getAvailability());
                break;
        }
    }
}

void Conversation::addMessage(const Shared::Message& data)
{
    int pos = m_ui->scrollArea->verticalScrollBar()->sliderPosition();
    int max = m_ui->scrollArea->verticalScrollBar()->maximum();
    MessageLine::Position place = line->message(data);
    if (place == MessageLine::invalid) {
        return;
    }
    
    if (scroll == nothing) {
        if (pos == max) {
            scroll = down;
        } else if (place != MessageLine::end) {         //todo make some better handling of that situation
            scroll = keep;
        }
    }
    if (!data.getOutgoing()) {
        const QString& res = data.getPenPalResource();
        if (res.size() > 0) {
            setPalResource(res);
        }
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
        const QString& aJid = contact->getAccountJid();
        m_ui->messageEditor->clear();
        Shared::Message msg(Shared::Message::chat);
        msg.setFromJid(aJid);
        msg.setFromResource(contact->getAccountResource());
        msg.setToJid(contact->getJid());
        msg.setToResource(activePalResource);
        msg.setBody(body);
        msg.setOutgoing(true);
        msg.generateRandomId();
        addMessage(msg);
        emit sendMessage(msg);
    }
}

void Conversation::onMessagesResize(int amount)
{
    switch (scroll) {
        case down:
            m_ui->scrollArea->verticalScrollBar()->setValue(m_ui->scrollArea->verticalScrollBar()->maximum());
            break;
        case keep:
            m_ui->scrollArea->verticalScrollBar()->setValue(m_ui->scrollArea->verticalScrollBar()->value() - amount);
            break;
        default:
            break;
    }
    scroll = nothing;
}

