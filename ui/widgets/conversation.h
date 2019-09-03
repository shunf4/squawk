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

#ifndef CONVERSATION_H
#define CONVERSATION_H

#include <QWidget>
#include <QScopedPointer>
#include "../../global.h"
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
    Conversation(bool muc, const QString& mJid, const QString mRes, const QString pJid, const QString pRes, const QString& acc, QWidget* parent = 0);
    ~Conversation();
    
    QString getJid() const;
    QString getAccount() const;
    QString getPalResource() const;
    virtual void addMessage(const Shared::Message& data);
    
    void setPalResource(const QString& res);
    void responseArchive(const std::list<Shared::Message> list);
    void showEvent(QShowEvent * event) override;
    
signals:
    void sendMessage(const Shared::Message& message);
    void requestArchive(const QString& before);
    void shown();
    
protected:
    virtual void setName(const QString& name);
    void applyVisualEffects();
    virtual void handleSendMessage(const QString& text) = 0;
    void setStatus(const QString& status);
    
protected slots:
    void onEnterPressed();
    void onMessagesResize(int amount);
    void onSliderValueChanged(int value);
    void onAttach();
    void onFileSelected();
    
public:
    const bool isMuc;
    
protected:
    enum Scroll {
        nothing,
        keep,
        down
    };
    QString myJid;
    QString myResource;
    QString palJid;
    QString activePalResource;
    QString account;
    MessageLine* line;
    QScopedPointer<Ui::Conversation> m_ui;
    KeyEnterReceiver ker;
    QString thread;
    QLabel* statusIcon;
    QLabel* statusLabel;
    Scroll scroll;
    bool manualSliderChange;
    bool requestingHistory;
    bool everShown;
};

#endif // CONVERSATION_H
