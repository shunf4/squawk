#include "contact.h"

Models::Contact::Contact(const QMap<QString, QVariant>& data, Models::Item* parentItem):
    Item(Item::contact, data, parentItem),
    jid(data.value("jid").toString()),
    state(data.value("state").toInt())
{
}

Models::Contact::~Contact()
{
}

QString Models::Contact::getJid() const
{
    return jid;
}

void Models::Contact::setJid(const QString p_jid)
{
    if (jid != p_jid) {
        jid = p_jid;
        emit changed(1);
    }
}

int Models::Contact::getState() const
{
    return state;
}

void Models::Contact::setState(int p_state)
{
    if (state != p_state) {
        state = p_state;
        emit changed(2);
    }
}

int Models::Contact::columnCount() const
{
    return 3;
}

QVariant Models::Contact::data(int column) const
{
    switch (column) {
        case 0:
            if (name == "") {
                return jid;
            } else {
                return Item::data(column);
            }
        case 1:
            return jid;
        case 2:
            return state;
        default:
            return QVariant();
    }
}

void Models::Contact::update(const QString& field, const QVariant& value)
{
    if (field == "name") {
        setName(value.toString());
    } else if (field == "jid") {
        setJid(value.toString());
    } else if (field == "state") {
        setState(value.toInt());
    }
}
