#ifndef ACCOUNT_H
#define ACCOUNT_H

#include <QScopedPointer>
#include <QDialog>
#include <QMap>
#include <QString>
#include <QVariant>

namespace Ui
{
class Account;
}

class Account : public QDialog
{
    Q_OBJECT

public:
    Account();
    ~Account();
    
    QMap<QString, QVariant> value() const;
    void setData(const QMap<QString, QVariant>& data);
    void lockId();

private:
    QScopedPointer<Ui::Account> m_ui;
};

#endif // ACCOUNT_H
