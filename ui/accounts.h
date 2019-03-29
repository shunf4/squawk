#ifndef ACCOUNTS_H
#define ACCOUNTS_H

#include <QWidget>
#include <QScopedPointer>

namespace Ui
{
class Accounts;
}

/**
 * @todo write docs
 */
class Accounts : public QWidget
{
    Q_OBJECT
public:
    explicit Accounts(QWidget *parent = nullptr);
    ~Accounts() override;

private:
    QScopedPointer<Ui::Accounts> m_ui;
};

#endif // ACCOUNTS_H
