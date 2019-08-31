/*
 * Squawk messenger. 
 * Copyright (C) 2019  Yury Gubich <blue@macaw.me>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "squawk.h"
#include "ui_squawk.h"
#include <QDebug>
#include <QIcon>

Squawk::Squawk(QWidget *parent) :
    QMainWindow(parent),
    m_ui(new Ui::Squawk),
    accounts(0),
    rosterModel(),
    conversations(),
    contextMenu(new QMenu()),
    dbus("org.freedesktop.Notifications", "/org/freedesktop/Notifications", "org.freedesktop.Notifications", QDBusConnection::sessionBus())
{
    m_ui->setupUi(this);
    m_ui->roster->setModel(&rosterModel);
    m_ui->roster->setContextMenuPolicy(Qt::CustomContextMenu);
    
    for (unsigned int i = Shared::availabilityLowest; i < Shared::availabilityHighest + 1; ++i) {
        Shared::Availability av = static_cast<Shared::Availability>(i);
        m_ui->comboBox->addItem(Shared::availabilityIcon(av), Shared::availabilityNames[av]);
    }
    m_ui->comboBox->setCurrentIndex(Shared::offline);
    
    connect(m_ui->actionAccounts, SIGNAL(triggered()), this, SLOT(onAccounts()));
    connect(m_ui->actionAddContact, SIGNAL(triggered()), this, SLOT(onNewContact()));
    connect(m_ui->comboBox, SIGNAL(activated(int)), this, SLOT(onComboboxActivated(int)));
    connect(m_ui->roster, SIGNAL(doubleClicked(const QModelIndex&)), this, SLOT(onRosterItemDoubleClicked(const QModelIndex&)));
    connect(m_ui->roster, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(onRosterContextMenu(const QPoint&)));
    
    connect(rosterModel.accountsModel, SIGNAL(sizeChanged(unsigned int)), this, SLOT(onAccountsSizeChanged(unsigned int)));
    //m_ui->mainToolBar->addWidget(m_ui->comboBox);
}

Squawk::~Squawk() {
    delete contextMenu;
}

void Squawk::onAccounts()
{
    if (accounts == 0) {
        accounts = new Accounts(rosterModel.accountsModel, this);
        accounts->setAttribute(Qt::WA_DeleteOnClose);
        connect(accounts, SIGNAL(destroyed(QObject*)), this, SLOT(onAccountsClosed(QObject*)));
        connect(accounts, SIGNAL(newAccount(const QMap<QString, QVariant>&)), this, SIGNAL(newAccountRequest(const QMap<QString, QVariant>&)));
        connect(accounts, SIGNAL(changeAccount(const QString&, const QMap<QString, QVariant>&)), this, SIGNAL(modifyAccountRequest(const QString&, const QMap<QString, QVariant>&)));
        connect(accounts, SIGNAL(connectAccount(const QString&)), this, SIGNAL(connectAccount(const QString&)));
        connect(accounts, SIGNAL(disconnectAccount(const QString&)), this, SIGNAL(disconnectAccount(const QString&)));
        connect(accounts, SIGNAL(removeAccount(const QString&)), this, SIGNAL(removeAccountRequest(const QString&)));
        
        accounts->show();
    } else {
        accounts->show();
        accounts->raise();
        accounts->activateWindow();
    }
}

void Squawk::onAccountsSizeChanged(unsigned int size)
{
    if (size > 0) {
        m_ui->actionAddContact->setEnabled(true);
    } else {
        m_ui->actionAddContact->setEnabled(false);
    }
}

void Squawk::onNewContact()
{
    NewContact* nc = new NewContact(rosterModel.accountsModel, this);
    
    connect(nc, SIGNAL(accepted()), this, SLOT(onNewContactAccepted()));
    connect(nc, SIGNAL(rejected()), nc, SLOT(deleteLater()));
    
    nc->exec();
}

void Squawk::onNewContactAccepted()
{
    NewContact* nc = static_cast<NewContact*>(sender());
    NewContact::Data value = nc->value();
    
    emit addContactRequest(value.account, value.jid, value.name, value.groups);
    
    nc->deleteLater();
}

void Squawk::closeEvent(QCloseEvent* event)
{
    if (accounts != 0) {
        accounts->close();
    }
    for (Conversations::const_iterator itr = conversations.begin(), end = conversations.end(); itr != end; ++itr) {
        disconnect(itr->second, SIGNAL(destroyed(QObject*)), this, SLOT(onConversationClosed(QObject*)));
        itr->second->close();
    }
    conversations.clear();
    
    QMainWindow::closeEvent(event);
}


void Squawk::onAccountsClosed(QObject* parent)
{
    accounts = 0;
}

void Squawk::newAccount(const QMap<QString, QVariant>& account)
{
    rosterModel.addAccount(account);
}

void Squawk::onComboboxActivated(int index)
{
    if (index != Shared::offline) {
        int size = rosterModel.accountsModel->rowCount(QModelIndex());
        if (size > 0) {
            emit changeState(index);
            for (int i = 0; i < size; ++i) {
                Models::Account* acc = rosterModel.accountsModel->getAccount(i);
                if (acc->getState() == Shared::disconnected) {
                    emit connectAccount(acc->getName());
                }
            }
        } else {
            m_ui->comboBox->setCurrentIndex(Shared::offline);
        }
    } else {
        emit changeState(index);
        int size = rosterModel.accountsModel->rowCount(QModelIndex());
        for (int i = 0; i != size; ++i) {
            Models::Account* acc = rosterModel.accountsModel->getAccount(i);
            if (acc->getState() != Shared::disconnected) {
                emit disconnectAccount(acc->getName());
            }
        }
    }
}

void Squawk::changeAccount(const QString& account, const QMap<QString, QVariant>& data)
{
    for (QMap<QString, QVariant>::const_iterator itr = data.begin(), end = data.end(); itr != end; ++itr) {
        QString attr = itr.key();
        rosterModel.updateAccount(account, attr, *itr);
    }
}

void Squawk::addContact(const QString& account, const QString& jid, const QString& group, const QMap<QString, QVariant>& data)
{
    rosterModel.addContact(account, jid, group, data);
}

void Squawk::addGroup(const QString& account, const QString& name)
{
    rosterModel.addGroup(account, name);
}

void Squawk::removeGroup(const QString& account, const QString& name)
{
    rosterModel.removeGroup(account, name);
}

void Squawk::changeContact(const QString& account, const QString& jid, const QMap<QString, QVariant>& data)
{
    rosterModel.changeContact(account, jid, data);
}

void Squawk::removeContact(const QString& account, const QString& jid)
{
    rosterModel.removeContact(account, jid);
}

void Squawk::removeContact(const QString& account, const QString& jid, const QString& group)
{
    rosterModel.removeContact(account, jid, group);
}

void Squawk::addPresence(const QString& account, const QString& jid, const QString& name, const QMap<QString, QVariant>& data)
{
    rosterModel.addPresence(account, jid, name, data);
}

void Squawk::removePresence(const QString& account, const QString& jid, const QString& name)
{
    rosterModel.removePresence(account, jid, name);
}

void Squawk::stateChanged(int state)
{
    m_ui->comboBox->setCurrentIndex(state);
}

void Squawk::onRosterItemDoubleClicked(const QModelIndex& item)
{
    if (item.isValid()) {
        Models::Item* node = static_cast<Models::Item*>(item.internalPointer());
        Models::Contact* contact = 0;
        Models::Room* room = 0;
        QString res;
        Models::Roster::ElId* id = 0;
        switch (node->type) {
            case Models::Item::contact:
                contact = static_cast<Models::Contact*>(node);
                id = new Models::Roster::ElId(contact->getAccountName(), contact->getJid());
                break;
            case Models::Item::presence:
                contact = static_cast<Models::Contact*>(node->parentItem());
                id = new Models::Roster::ElId(contact->getAccountName(), contact->getJid());
                res = node->getName();
                break;
            case Models::Item::room:
                room = static_cast<Models::Room*>(node);
                id = new Models::Roster::ElId(room->getAccountName(), room->getJid());
                break;
            default:
                m_ui->roster->expand(item);
                break;
        }
        
        if (id != 0) {
            Conversations::const_iterator itr = conversations.find(*id);
            Conversation* conv = 0;
            bool created = false;
            if (itr != conversations.end()) {
                conv = itr->second;
            } else if (contact != 0) {
                created = true;
                conv = new Chat(contact);
            } else if (room != 0) {
                created = true;
                conv = new Room(room);
                
                if (!room->getJoined()) {
                    emit setRoomJoined(id->account, id->name, true);
                }
            }
            
            if (conv != 0) {
                if (created) {
                    conv->setAttribute(Qt::WA_DeleteOnClose);
                    
                    connect(conv, SIGNAL(destroyed(QObject*)), this, SLOT(onConversationClosed(QObject*)));
                    connect(conv, SIGNAL(sendMessage(const Shared::Message&)), this, SLOT(onConversationMessage(const Shared::Message&)));
                    connect(conv, SIGNAL(requestArchive(const QString&)), this, SLOT(onConversationRequestArchive(const QString&)));
                    connect(conv, SIGNAL(shown()), this, SLOT(onConversationShown()));
                    
                    conversations.insert(std::make_pair(*id, conv));
                }
                
                conv->show();
                conv->raise();
                conv->activateWindow();
                
                if (res.size() > 0) {
                    conv->setPalResource(res);
                }
            }
            
        }
    }
}

void Squawk::onConversationShown()
{
    Conversation* conv = static_cast<Conversation*>(sender());
    rosterModel.dropMessages(conv->getAccount(), conv->getJid());
}

void Squawk::onConversationClosed(QObject* parent)
{
    Conversation* conv = static_cast<Conversation*>(sender());
    Models::Roster::ElId id(conv->getAccount(), conv->getJid());
    Conversations::const_iterator itr = conversations.find(id);
    if (itr == conversations.end()) {
        qDebug() << "Conversation has been closed but can not be found among other opened conversations, application is most probably going to crash";
        return;
    }
    if (conv->isMuc) {
        Room* room = static_cast<Room*>(conv);
        if (!room->autoJoined()) {
            emit setRoomJoined(id.account, id.name, false);
        }
    }
    conversations.erase(itr);
}

void Squawk::accountMessage(const QString& account, const Shared::Message& data)
{
    const QString& from = data.getPenPalJid();
    Conversations::iterator itr = conversations.find({account, from});
    if (itr != conversations.end()) {
        Conversation* conv = itr->second;
        conv->addMessage(data);
        QApplication::alert(conv);
        if (conv->isMinimized()) {
            rosterModel.addMessage(account, data);
            if (!data.getForwarded()) {
                notify(account, data);
            }
        }
    } else {
        rosterModel.addMessage(account, data);
        if (!data.getForwarded()) {
            QApplication::alert(this);
            notify(account, data);
        }
    }
}

void Squawk::notify(const QString& account, const Shared::Message& msg)
{
    QString name = QString(rosterModel.getContactName(account, msg.getPenPalJid()));;
    QVariantList args;
    args << QString(QCoreApplication::applicationName());
    args << QVariant(QVariant::UInt);   //TODO some normal id
    args << QString("mail-message");    //TODO icon
    if (msg.getType() == Shared::Message::groupChat) {
        args << msg.getFromResource() + " from " + name;
    } else {
        args << name;
    }
    args << QString(msg.getBody());
    args << QStringList();
    args << QVariantMap();
    args << 3000;
    dbus.callWithArgumentList(QDBus::AutoDetect, "Notify", args);
}

void Squawk::onConversationMessage(const Shared::Message& msg)
{
    Conversation* conv = static_cast<Conversation*>(sender());
    
    emit sendMessage(conv->getAccount(), msg);
}

void Squawk::onConversationRequestArchive(const QString& before)
{
    Conversation* conv = static_cast<Conversation*>(sender());
    requestArchive(conv->getAccount(), conv->getJid(), 20, before); //TODO amount as a settings value
}

void Squawk::responseArchive(const QString& account, const QString& jid, const std::list<Shared::Message>& list)
{
    Models::Roster::ElId id(account, jid);
    
    Conversations::const_iterator itr = conversations.find(id);
    if (itr != conversations.end()) {
        itr->second->responseArchive(list);
    }
}

void Squawk::removeAccount(const QString& account)
{
    Conversations::const_iterator itr = conversations.begin();
    while (itr != conversations.end()) {
        if (itr->first.account == account) {
            Conversations::const_iterator lItr = itr;
            ++itr;
            Conversation* conv = lItr->second;
            disconnect(conv, SIGNAL(destroyed(QObject*)), this, SLOT(onConversationClosed(QObject*)));
            disconnect(conv, SIGNAL(sendMessage(const Shared::Message&)), this, SLOT(onConversationMessage(const Shared::Message&)));
            disconnect(conv, SIGNAL(requestArchive(const QString&)), this, SLOT(onConversationRequestArchive(const QString&)));
            disconnect(conv, SIGNAL(shown()), this, SLOT(onConversationShown()));
            conv->close();
            conversations.erase(lItr);
        } else {
            ++itr;
        }
    }
    rosterModel.removeAccount(account);
}

void Squawk::onRosterContextMenu(const QPoint& point)
{
    QModelIndex index = m_ui->roster->indexAt(point);
    if (index.isValid()) {
        Models::Item* item = static_cast<Models::Item*>(index.internalPointer());
    
        contextMenu->clear();
        bool hasMenu = false;
        switch (item->type) {
            case Models::Item::account: {
                Models::Account* acc = static_cast<Models::Account*>(item);
                hasMenu = true;
                QString name = acc->getName();
                
                if (acc->getState() != Shared::disconnected) {
                    QAction* con = contextMenu->addAction(Shared::icon("network-disconnect"), "Disconnect");
                    connect(con, &QAction::triggered, [this, name]() {
                        emit disconnectAccount(name);
                    });
                } else {
                    QAction* con = contextMenu->addAction(Shared::icon("network-connect"), "Connect");
                    connect(con, &QAction::triggered, [this, name]() {
                        emit connectAccount(name);
                    });
                }
                
                QAction* remove = contextMenu->addAction(Shared::icon("edit-delete"), "Remove");
                connect(remove, &QAction::triggered, [this, name]() {
                    emit removeAccount(name);
                });
                
            }
                break;
            case Models::Item::contact: {
                Models::Contact* cnt = static_cast<Models::Contact*>(item);
                hasMenu = true;
                
                QAction* dialog = contextMenu->addAction(Shared::icon("mail-message"), "Open dialog");
                connect(dialog, &QAction::triggered, [this, index]() {
                    onRosterItemDoubleClicked(index);
                });
                
                Shared::SubscriptionState state = cnt->getState();
                switch (state) {
                    case Shared::both:
                    case Shared::to: {
                        QAction* unsub = contextMenu->addAction(Shared::icon("news-unsubscribe"), "Unsubscribe");
                        connect(unsub, &QAction::triggered, [this, cnt]() {
                            emit unsubscribeContact(cnt->getAccountName(), cnt->getJid(), "");
                        });
                    }
                    break;
                    case Shared::from:
                    case Shared::unknown:
                    case Shared::none: {
                        QAction* sub = contextMenu->addAction(Shared::icon("news-subscribe"), "Subscribe");
                        connect(sub, &QAction::triggered, [this, cnt]() {
                            emit subscribeContact(cnt->getAccountName(), cnt->getJid(), "");
                        });
                    }    
                }
                
                QAction* remove = contextMenu->addAction(Shared::icon("edit-delete"), "Remove");
                connect(remove, &QAction::triggered, [this, cnt]() {
                    emit removeContactRequest(cnt->getAccountName(), cnt->getJid());
                });
                
            }
                break;
            default:
                break;
        }
        if (hasMenu) {
            contextMenu->popup(m_ui->roster->viewport()->mapToGlobal(point));
        }
    }    
}

void Squawk::addRoom(const QString& account, const QString jid, const QMap<QString, QVariant>& data)
{
    rosterModel.addRoom(account, jid, data);
}

void Squawk::changeRoom(const QString& account, const QString jid, const QMap<QString, QVariant>& data)
{
    rosterModel.changeRoom(account, jid, data);
}

void Squawk::removeRoom(const QString& account, const QString jid)
{
    rosterModel.removeRoom(account, jid);
}
