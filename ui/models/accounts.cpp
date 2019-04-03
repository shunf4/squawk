#include "accounts.h"
#include "../../global.h"

#include <QIcon>

std::deque<QString> Models::Accounts::columns = {
    "name",
    "server",
    "state"
};

Models::Accounts::Accounts(QObject* parent):
    QAbstractTableModel(parent),
    accs()
{

}

Models::Accounts::~Accounts()
{

}

QVariant Models::Accounts::data (const QModelIndex& index, int role) const
{
    QVariant answer;
    switch (role) {
        case Qt::DisplayRole: 
            answer = accs[index.row()]->data(index.column());
            break;
        case Qt::DecorationRole:
            if (index.column() == 2) {
                answer = QIcon::fromTheme(Shared::ConnectionStateThemeIcons[accs[index.row()]->getState()]);
            }
            break;
        default:
            break;
    }
    
    return answer;
}

int Models::Accounts::columnCount ( const QModelIndex& parent ) const
{
    return columns.size();
}

int Models::Accounts::rowCount ( const QModelIndex& parent ) const
{
    return accs.size();
}

QVariant Models::Accounts::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        return columns[section];
    }
    return QVariant();
}


void Models::Accounts::addAccount(Account* account)
{
    beginInsertRows(QModelIndex(), accs.size(), accs.size());
    accs.push_back(account);
    connect(account, SIGNAL(changed(int)), this, SLOT(onAccountChanged(int)));
    endInsertRows();
}

void Models::Accounts::onAccountChanged(int column)
{
    Account* acc = static_cast<Account*>(sender());
    
    if (column < columnCount(QModelIndex())) {
        int row = acc->row();
        emit dataChanged(createIndex(row, column, this), createIndex(row, column, this));
    }
}

Models::Account * Models::Accounts::getAccount(int index)
{
    return accs[index];
}
