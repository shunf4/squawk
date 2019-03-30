#include "accounts.h"

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

QVariant Models::Accounts::data ( const QModelIndex& index, int role ) const
{
    QVariant answer;
    switch (role) {
        case Qt::DisplayRole: {
            const Account& acc = accs[index.row()];
            switch (index.column()) {
                case 0:
                    answer = acc.name;
                    break;
                case 1:
                    answer = acc.server;
                    break;
                case 2:
                    answer = acc.state;
                break;
            }
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


void Models::Accounts::addAccount(const QMap<QString, QVariant>& map)
{
    beginInsertRows(QModelIndex(), accs.size(), accs.size());
    accs.push_back({
        map.value("name").toString(),
        map.value("server").toString(),
        map.value("login").toString(),
        map.value("password").toString(),
        map.value("state").toInt()
    });
    endInsertRows();
}
