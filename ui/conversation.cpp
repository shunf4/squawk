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

Conversation::Conversation(Models::Contact* p_contact, QWidget* parent):
    QWidget(parent),
    contact(p_contact),
    m_ui(new Ui::Conversation),
    ker()
{
    m_ui->setupUi(this);
    m_ui->splitter->setSizes({300, 0});
    m_ui->splitter->setStretchFactor(1, 0);
    
    setName(p_contact->getName());
    setState(p_contact->getAvailability());
    
    connect(contact, SIGNAL(childChanged(Models::Item*, int, int)), this, SLOT(onContactChanged(Models::Item*, int, int)));
    connect(&ker, SIGNAL(enterPressed()), this, SLOT(onEnterPressed()));
    
    m_ui->dialogBox->installEventFilter(&ker);
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

void Conversation::addMessage(const QMap<QString, QString>& data)
{
    m_ui->dialogBox->append(data.value("from") + ": " + data.value("body"));
}

bool KeyEnterReceiver::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type()==QEvent::KeyPress) {
        QKeyEvent* key = static_cast<QKeyEvent*>(event);
        if ( (key->key()==Qt::Key_Enter) || (key->key()==Qt::Key_Return) ) {
            emit enterPressed();
        } else {
            return QObject::eventFilter(obj, event);
        }
        return true;
    } else {
        return QObject::eventFilter(obj, event);
    }
    return false;
}

void Conversation::onEnterPressed()
{
    qDebug() << "enter";
}
