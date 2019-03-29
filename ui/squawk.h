#ifndef SQUAWK_H
#define SQUAWK_H

#include <QMainWindow>
#include <QScopedPointer>
#include <QCloseEvent>

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

private:
    QScopedPointer<Ui::Squawk> m_ui;
    
    Accounts* accounts;
    
protected:
    void closeEvent(QCloseEvent * event) override;
    
private slots:
    void onAccounts();
    void onAccountsClosed(QObject* parent = 0);
};

#endif // SQUAWK_H
