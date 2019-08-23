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

#ifndef SQUAWK_H
#define SQUAWK_H

#include <QMainWindow>
#include <QScopedPointer>
#include <QCloseEvent>
#include <QtDBus/QDBusInterface>
#include <deque>
#include <map>
#include <list>

#include "accounts.h"
#include "widgets/chat.h"
#include "models/roster.h"
#include "newcontact.h"

#include "../global.h"

namespace Ui {
class Squawk;
}

class Squawk : public QMainWindow
{
    Q_OBJECT

public:
    explicit Squawk(QWidget *parent = nullptr);
    ~Squawk() override;
    
signals:
    void newAccountRequest(const QMap<QString, QVariant>&);
    void modifyAccountRequest(const QString&, const QMap<QString, QVariant>&);
    void removeAccountRequest(const QString&);
    void connectAccount(const QString&);
    void disconnectAccount(const QString&);
    void changeState(int state);
    void sendMessage(const QString& account, const Shared::Message& data);
    void requestArchive(const QString& account, const QString& jid, int count, const QString& before);
    void subscribeContact(const QString& account, const QString& jid, const QString& reason);
    void unsubscribeContact(const QString& account, const QString& jid, const QString& reason);
    void removeContactRequest(const QString& account, const QString& jid);
    void addContactRequest(const QString& account, const QString& jid, const QString& name, const QSet<QString>& groups);
    
public slots:
    void newAccount(const QMap<QString, QVariant>& account);
    void changeAccount(const QString& account, const QMap<QString, QVariant>& data);
    void removeAccount(const QString& account);
    void addGroup(const QString& account, const QString& name);
    void removeGroup(const QString& account, const QString& name);
    void addContact(const QString& account, const QString& jid, const QString& group, const QMap<QString, QVariant>& data);
    void removeContact(const QString& account, const QString& jid, const QString& group);
    void removeContact(const QString& account, const QString& jid);
    void changeContact(const QString& account, const QString& jid, const QMap<QString, QVariant>& data);
    void addPresence(const QString& account, const QString& jid, const QString& name, const QMap<QString, QVariant>& data);
    void removePresence(const QString& account, const QString& jid, const QString& name);
    void stateChanged(int state);
    void accountMessage(const QString& account, const Shared::Message& data);
    void responseArchive(const QString& account, const QString& jid, const std::list<Shared::Message>& list);
    void addRoom(const QString& account, const QString jid, const QMap<QString, QVariant>& data);
    void changeRoom(const QString& account, const QString jid, const QMap<QString, QVariant>& data);
    void removeRoom(const QString& account, const QString jid);
    
private:
    typedef std::map<Models::Roster::ElId, Conversation*> Conversations;
    QScopedPointer<Ui::Squawk> m_ui;
    
    Accounts* accounts;
    Models::Roster rosterModel;
    Conversations conversations;
    QMenu* contextMenu;
    QDBusInterface dbus;
    
protected:
    void closeEvent(QCloseEvent * event) override;
    void notify(const QString& account, const Shared::Message& msg);
    
private slots:
    void onAccounts();
    void onNewContact();
    void onNewContactAccepted();
    void onAccountsSizeChanged(unsigned int size);
    void onAccountsClosed(QObject* parent = 0);
    void onConversationClosed(QObject* parent = 0);
    void onComboboxActivated(int index);
    void onRosterItemDoubleClicked(const QModelIndex& item);
    void onConversationMessage(const Shared::Message& msg);
    void onConversationRequestArchive(const QString& before);
    void onRosterContextMenu(const QPoint& point);
    void onConversationShown();
    
};

#endif // SQUAWK_H
