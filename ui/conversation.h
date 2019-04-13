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

#ifndef CONVERSATION_H
#define CONVERSATION_H

#include <QWidget>
#include <QScopedPointer>
#include "../global.h"
#include "models/contact.h"
#include "messageline.h"

namespace Ui
{
class Conversation;
}

class KeyEnterReceiver : public QObject
{
    Q_OBJECT
public:
    KeyEnterReceiver(QObject* parent = 0);
protected:
    bool ownEvent;
    bool eventFilter(QObject* obj, QEvent* event);
    
signals:
    void enterPressed();
};

class Conversation : public QWidget
{
    Q_OBJECT
public:
    Conversation(Models::Contact* p_contact, QWidget* parent = 0);
    ~Conversation();
    
    QString getJid() const;
    QString getAccount() const;
    QString getPalResource() const;
    void addMessage(const Shared::Message& data);
    
    void setPalResource(const QString& res);
    
signals:
    void sendMessage(const Shared::Message& message);
    
protected:
    void setState(Shared::Availability state);
    void setStatus(const QString& status);
    void setName(const QString& name);
    
protected slots:
    void onContactChanged(Models::Item* item, int row, int col);
    void onEnterPressed();
    void onMessagesResize(int amount);
    
private:
    enum Scroll {
        nothing,
        keep,
        down
    };
    Models::Contact* contact;
    MessageLine* line;
    QScopedPointer<Ui::Conversation> m_ui;
    KeyEnterReceiver ker;
    QString activePalResource;
    QString thread;
    Scroll scroll;
};

#endif // CONVERSATION_H
