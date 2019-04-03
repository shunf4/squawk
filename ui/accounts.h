#ifndef ACCOUNTS_H
#define ACCOUNTS_H

#include <QWidget>
#include <QScopedPointer>

#include "account.h"
#include "models/accounts.h"

namespace Ui
{
class Accounts;
}

class Accounts : public QWidget
{
    Q_OBJECT
public:
    explicit Accounts(Models::Accounts* model, QWidget *parent = nullptr);
    ~Accounts() override;
    
signals:
    void newAccount(const QMap<QString, QVariant>&);
    
private slots:
    void onAddButton(bool clicked = 0);
    void onAccountAccepted();
    void onAccountRejected();
    
private:
    QScopedPointer<Ui::Accounts> m_ui;
};

#endif // ACCOUNTS_H
