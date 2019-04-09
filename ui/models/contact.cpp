#include "contact.h"
#include <QDebug>

Models::Contact::Contact(const QString& p_jid ,const QMap<QString, QVariant> &data, Item *parentItem):
    Item(Item::contact, data, parentItem),
    jid(p_jid),
    availability(Shared::offline),
    state(Shared::none),
    presences()
{
    QMap<QString, QVariant>::const_iterator itr = data.find("state");
    if (itr != data.end()) {
        setState(itr.value().toUInt());
    }
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
        changed(1);
    }
}

void Models::Contact::setAvailability(unsigned int p_state)
{
    if (p_state <= Shared::availabilityHighest) {
        Shared::Availability state = static_cast<Shared::Availability>(p_state);
        setAvailability(state);
    } else {
        qDebug() << "An attempt to set invalid availability " << p_state << " to the contact " << jid;
    }
}

void Models::Contact::setState(unsigned int p_state)
{
    if (p_state <= Shared::subscriptionStateHighest) {
        Shared::SubscriptionState state = static_cast<Shared::SubscriptionState>(p_state);
        setState(state);
    } else {
        qDebug() << "An attempt to set invalid subscription state " << p_state << " to the contact " << jid;
    }
}

Shared::Availability Models::Contact::getAvailability() const
{
    return availability;
}

void Models::Contact::setAvailability(Shared::Availability p_state)
{
    if (availability != p_state) {
        availability = p_state;
        changed(3);
    }
}

int Models::Contact::columnCount() const
{
    return 4;
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
        case 3:
            return availability;
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
    } else if (field == "availability") {
        setAvailability(value.toUInt());
    } else if (field == "state") {
        setState(value.toUInt());
    }
}

void Models::Contact::addPresence(const QString& p_name, const QMap<QString, QVariant>& data)
{
    QMap<QString, Presence*>::iterator itr = presences.find(p_name);
    
    if (itr == presences.end()) {
        Presence* pr = new Presence(data);
        pr->setName(p_name);
        presences.insert(p_name, pr);
        appendChild(pr);
    } else {
        Presence* pr = itr.value();
        for (QMap<QString, QVariant>::const_iterator itr = data.begin(), end = data.end(); itr != end; ++itr) {
            pr->update(itr.key(), itr.value());
        }
    }
}

void Models::Contact::removePresence(const QString& name)
{
    QMap<QString, Presence*>::iterator itr = presences.find(name);
    
    if (itr == presences.end()) {
        qDebug() << "an attempt to remove non existing presence " << name << " from the contact " << jid << " of account " << getAccountName() << ", skipping";
    } else {
        Presence* pr = itr.value();
        presences.erase(itr);
        removeChild(pr->row());
    }
}

void Models::Contact::refresh()
{
    QDateTime lastActivity;
    Presence* presence = 0;
    for (QMap<QString, Presence*>::iterator itr = presences.begin(), end = presences.end(); itr != end; ++itr) {
        Presence* pr = itr.value();
        QDateTime la = pr->getLastActivity();
        
        if (la > lastActivity) {
            lastActivity = la;
            presence = pr;
        }
    }
    
    if (presence != 0) {
        setAvailability(presence->getAvailability());
    } else {
        setAvailability(Shared::offline);
    }
}

void Models::Contact::_removeChild(int index)
{
    Item* child = childItems[index];
    disconnect(child, SIGNAL(childChanged(Models::Item*, int, int)), this, SLOT(refresh()));
    Item::_removeChild(index);
    refresh();
}

void Models::Contact::appendChild(Models::Item* child)
{
    Item::appendChild(child);
    connect(child, SIGNAL(childChanged(Models::Item*, int, int)), this, SLOT(refresh()));
    refresh();
}

Shared::SubscriptionState Models::Contact::getState() const
{
    return state;
}

void Models::Contact::setState(Shared::SubscriptionState p_state)
{
    if (state != p_state) {
        state = p_state;
        changed(2);
    }
}

QIcon Models::Contact::getStatusIcon() const
{
    if (state == Shared::both) {
        return QIcon::fromTheme(Shared::availabilityThemeIcons[availability]);
    } else {
        return QIcon::fromTheme(Shared::subscriptionStateThemeIcons[state]);
    }
}

QString Models::Contact::getAccountName() const
{
    const Item* p = this;
    do {
        p = p->parentItemConst();
    } while (p != 0 && p->type != Item::account);
    
    if (p == 0) {
        qDebug() << "An attempt to request account name of the contact " << jid << " but the parent account wasn't found, returning empty string";
        return "";
    }
    return p->getName();
}

