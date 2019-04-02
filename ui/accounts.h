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
    explicit Accounts(QWidget *parent = nullptr);
    ~Accounts() override;

    void addAccount(const QMap<QString, QVariant>&);
    void updateAccount(const QString& account, const QString& field, const QVariant& value);
    
signals:
    void newAccount(const QMap<QString, QVariant>&);
    
private slots:
    void onAddButton(bool clicked = 0);
    void onAccountAccepted();
    void onAccountRejected();
    
private:
    QScopedPointer<Ui::Accounts> m_ui;
    Models::Accounts tableModel;
};

#endif // ACCOUNTS_H
