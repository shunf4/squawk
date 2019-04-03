#ifndef MODELS_ACCOUNTS_H
#define MODELS_ACCOUNTS_H

#include <qabstractitemmodel.h>
#include <deque>

namespace Models
{

class Accounts : public QAbstractTableModel
{
    Q_OBJECT
public:
    Accounts(QObject* parent = 0);
    ~Accounts();
    
    void addAccount(const QMap<QString, QVariant>& map);
    void updateAccount(const QString& account, const QString& field, const QVariant& value);
    
    QVariant data ( const QModelIndex& index, int role ) const override;
    int columnCount ( const QModelIndex& parent ) const override;
    int rowCount ( const QModelIndex& parent ) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

private:
    std::deque<Account*> accs;
    
    static std::deque<QString> columns;
    
};
}

#endif // MODELS_ACCOUNT_H
