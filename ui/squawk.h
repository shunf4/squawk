#ifndef SQUAWK_H
#define SQUAWK_H

#include <QMainWindow>
#include <QScopedPointer>
#include <QCloseEvent>
#include <deque>

#include "accounts.h"
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
    
public slots:
    void newAccount(const QMap<QString, QVariant>& account);
    void accountConnectionStateChanged(const QString& account, int state);
    
private:
    typedef std::deque<QMap<QString, QVariant>> AC;
    QScopedPointer<Ui::Squawk> m_ui;
    
    Accounts* accounts;
    AC accountsCache;
    Models::Roster rosterModel;
    
protected:
    void closeEvent(QCloseEvent * event) override;
    
private slots:
    void onAccounts();
    void onAccountsClosed(QObject* parent = 0);
    void onComboboxActivated(int index);
    
};

#endif // SQUAWK_H
