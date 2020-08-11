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
#include <QSettings>
#include <QInputDialog>

#include <deque>
#include <map>
#include <set>
#include <list>

#include "widgets/accounts.h"
#include "widgets/chat.h"
#include "widgets/room.h"
#include "widgets/newcontact.h"
#include "widgets/joinconference.h"
#include "models/roster.h"
#include "widgets/vcard/vcard.h"

#include "shared.h"

namespace Ui {
class Squawk;
}

class Squawk : public QMainWindow
{
    Q_OBJECT

public:
    explicit Squawk(QWidget *parent = nullptr);
    ~Squawk() override;
    
    void writeSettings();
    
signals:
    void newAccountRequest(const QMap<QString, QVariant>&);
    void modifyAccountRequest(const QString&, const QMap<QString, QVariant>&);
    void removeAccountRequest(const QString&);
    void connectAccount(const QString&);
    void disconnectAccount(const QString&);
    void changeState(Shared::Availability state);
    void sendMessage(const QString& account, const Shared::Message& data);
    void sendMessage(const QString& account, const Shared::Message& data, const QString& path);
    void requestArchive(const QString& account, const QString& jid, int count, const QString& before);
    void subscribeContact(const QString& account, const QString& jid, const QString& reason);
    void unsubscribeContact(const QString& account, const QString& jid, const QString& reason);
    void removeContactRequest(const QString& account, const QString& jid);
    void addContactRequest(const QString& account, const QString& jid, const QString& name, const QSet<QString>& groups);
    void addContactToGroupRequest(const QString& account, const QString& jid, const QString& groupName);
    void removeContactFromGroupRequest(const QString& account, const QString& jid, const QString& groupName);
    void renameContactRequest(const QString& account, const QString& jid, const QString& newName);
    void setRoomJoined(const QString& account, const QString& jid, bool joined);
    void setRoomAutoJoin(const QString& account, const QString& jid, bool joined);
    void addRoomRequest(const QString& account, const QString& jid, const QString& nick, const QString& password, bool autoJoin);
    void removeRoomRequest(const QString& account, const QString& jid);
    void fileLocalPathRequest(const QString& messageId, const QString& url);
    void downloadFileRequest(const QString& messageId, const QString& url);
    void requestVCard(const QString& account, const QString& jid);
    void uploadVCard(const QString& account, const Shared::VCard& card);
    void responsePassword(const QString& account, const QString& password);
    
public slots:
    void readSettings();
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
    void stateChanged(Shared::Availability state);
    void accountMessage(const QString& account, const Shared::Message& data);
    void responseArchive(const QString& account, const QString& jid, const std::list<Shared::Message>& list);
    void addRoom(const QString& account, const QString jid, const QMap<QString, QVariant>& data);
    void changeRoom(const QString& account, const QString jid, const QMap<QString, QVariant>& data);
    void removeRoom(const QString& account, const QString jid);
    void addRoomParticipant(const QString& account, const QString& jid, const QString& name, const QMap<QString, QVariant>& data);
    void changeRoomParticipant(const QString& account, const QString& jid, const QString& name, const QMap<QString, QVariant>& data);
    void removeRoomParticipant(const QString& account, const QString& jid, const QString& name);
    void fileLocalPathResponse(const QString& messageId, const QString& path);
    void fileError(const QString& messageId, const QString& error);
    void fileProgress(const QString& messageId, qreal value);
    void responseVCard(const QString& jid, const Shared::VCard& card);
    void changeMessage(const QString& account, const QString& jid, const QString& id, const QMap<QString, QVariant>& data);
    void requestPassword(const QString& account);
    
private:
    typedef std::map<Models::Roster::ElId, Conversation*> Conversations;
    QScopedPointer<Ui::Squawk> m_ui;
    
    Accounts* accounts;
    Models::Roster rosterModel;
    Conversations conversations;
    QMenu* contextMenu;
    QDBusInterface dbus;
    std::map<QString, std::set<Models::Roster::ElId>> requestedFiles;
    std::map<QString, VCard*> vCards;
    std::deque<QString> requestedAccountsForPasswords;
    QInputDialog* prompt;
    Conversation* currentConversation;
    QModelIndex restoreSelection;
    bool needToRestore;
    
protected:
    void closeEvent(QCloseEvent * event) override;
    void notify(const QString& account, const Shared::Message& msg);
    
private slots:
    void onAccounts();
    void onNewContact();
    void onNewConference();
    void onNewContactAccepted();
    void onJoinConferenceAccepted();
    void onAccountsSizeChanged(unsigned int size);
    void onAccountsClosed(QObject* parent = 0);
    void onConversationClosed(QObject* parent = 0);
    void onVCardClosed();
    void onVCardSave(const Shared::VCard& card, const QString& account);
    void onActivateVCard(const QString& account, const QString& jid, bool edition = false);
    void onComboboxActivated(int index);
    void onRosterItemDoubleClicked(const QModelIndex& item);
    void onConversationMessage(const Shared::Message& msg);
    void onConversationMessage(const Shared::Message& msg, const QString& path);
    void onConversationRequestArchive(const QString& account, const QString& jid, const QString& before);
    void onRosterContextMenu(const QPoint& point);
    void onConversationRequestLocalFile(const QString& messageId, const QString& url);
    void onConversationDownloadFile(const QString& messageId, const QString& url);
    void onItemCollepsed(const QModelIndex& index);
    void onPasswordPromptAccepted();
    void onPasswordPromptRejected();
    void onRosterSelectionChanged(const QModelIndex& current, const QModelIndex& previous);
    void onContextAboutToHide();
    
private:
    void checkNextAccountForPassword();
    void onPasswordPromptDone();
    void subscribeConversation(Conversation* conv);
};

#endif // SQUAWK_H
