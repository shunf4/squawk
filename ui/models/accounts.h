#ifndef MODELS_ACCOUNTS_H
#define MODELS_ACCOUNTS_H

#include <qabstractitemmodel.h>
#include <deque>
#include "account.h"

namespace Models
{

class Accounts : public QAbstractTableModel
{
    Q_OBJECT
public:
    Accounts(QObject* parent = 0);
    ~Accounts();
    
    void addAccount(Account* account);
    void removeAccount(int index);
    
    QVariant data ( const QModelIndex& index, int role ) const override;
    int columnCount ( const QModelIndex& parent ) const override;
    int rowCount ( const QModelIndex& parent ) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    Account* getAccount(int index);
    
    std::deque<QString> getNames() const;
    
signals:
    void changed();
    void sizeChanged(unsigned int size);
    
private:
    std::deque<Account*> accs;
    static std::deque<QString> columns;
    
private slots:
    void onAccountChanged(Models::Item* item, int row, int col);
    
};
}

#endif // MODELS_ACCOUNT_H
