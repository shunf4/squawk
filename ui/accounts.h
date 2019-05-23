#ifndef ACCOUNTS_H
#define ACCOUNTS_H

#include <QWidget>
#include <QScopedPointer>
#include <QItemSelection>

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
    explicit Accounts(Models::Accounts* p_model, QWidget *parent = nullptr);
    ~Accounts() override;
    
signals:
    void newAccount(const QMap<QString, QVariant>&);
    void changeAccount(const QMap<QString, QVariant>&);
    
private slots:
    void onAddButton(bool clicked = 0);
    void onEditButton(bool clicked = 0);
    void onAccountAccepted();
    void onAccountRejected();
    void onSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
    
private:
    QScopedPointer<Ui::Accounts> m_ui;
    Models::Accounts* model;
    bool editing;
};

#endif // ACCOUNTS_H
