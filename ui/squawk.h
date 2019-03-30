#ifndef SQUAWK_H
#define SQUAWK_H

#include <QMainWindow>
#include <QScopedPointer>
#include <QCloseEvent>
#include <deque>

#include "accounts.h"

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
    
public slots:
    void newAccount(const QMap<QString, QVariant>& account);
    
private:
    typedef std::deque<QMap<QString, QVariant>> AC;
    QScopedPointer<Ui::Squawk> m_ui;
    
    Accounts* accounts;
    AC accountsCache;
    
protected:
    void closeEvent(QCloseEvent * event) override;
    
private slots:
    void onAccounts();
    void onAccountsClosed(QObject* parent = 0);
};

#endif // SQUAWK_H
