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
                    answer = Shared::ConnectionStateNames[acc.state];
                break;
            }
        }
            break;
        case Qt::DecorationRole:
            if (index.column() == 2) {
                answer = QIcon::fromTheme(Shared::ConnectionStateThemeIcons[accs[index.row()].state]);
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

void Models::Accounts::updateAccount(const QString& account, const QString& field, const QVariant& value)
{
    for (int i = 0; i < accs.size(); ++i) {
        Account& acc = accs[i];
        if (acc.name == account) {
            if (field == "name") {
                acc.name = value.toString();
                emit dataChanged(createIndex(i, 0), createIndex(i, 0));
            } else if (field == "server") {
                acc.server = value.toString();
                emit dataChanged(createIndex(i, 1), createIndex(i, 1));
            } else if (field == "login") {
                acc.login = value.toString();
            } else if (field == "password") {
                acc.password = value.toString();
            } else if (field == "state") {
                acc.state = value.toInt();
                emit dataChanged(createIndex(i, 2), createIndex(i, 2));
            }
        }
    }
}

