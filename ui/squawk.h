#ifndef SQUAWK_H
#define SQUAWK_H

#include <QMainWindow>
#include <QScopedPointer>
#include <QCloseEvent>
#include <deque>
#include <map>

#include "accounts.h"
#include "conversation.h"
#include "models/roster.h"

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
    void connectAccount(const QString&);
    void disconnectAccount(const QString&);
    void changeState(int state);
    
public slots:
    void newAccount(const QMap<QString, QVariant>& account);
    void accountConnectionStateChanged(const QString& account, int state);
    void accountAvailabilityChanged(const QString& account, int state);
    void addGroup(const QString& account, const QString& name);
    void removeGroup(const QString& account, const QString& name);
    void addContact(const QString& account, const QString& jid, const QString& group, const QMap<QString, QVariant>& data);
    void removeContact(const QString& account, const QString& jid, const QString& group);
    void removeContact(const QString& account, const QString& jid);
    void changeContact(const QString& account, const QString& jid, const QMap<QString, QVariant>& data);
    void addPresence(const QString& account, const QString& jid, const QString& name, const QMap<QString, QVariant>& data);
    void removePresence(const QString& account, const QString& jid, const QString& name);
    void stateChanged(int state);
    
private:
    typedef std::map<Models::Roster::ElId, Conversation*> Conversations;
    QScopedPointer<Ui::Squawk> m_ui;
    
    Accounts* accounts;
    Models::Roster rosterModel;
    Conversations conversations;
    
protected:
    void closeEvent(QCloseEvent * event) override;
    
private slots:
    void onAccounts();
    void onAccountsClosed(QObject* parent = 0);
    void onConversationClosed(QObject* parent = 0);
    void onComboboxActivated(int index);
    void onRosterItemDoubleClicked(const QModelIndex& item);
    
};

#endif // SQUAWK_H
