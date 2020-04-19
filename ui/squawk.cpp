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
    dbus("org.freedesktop.Notifications", "/org/freedesktop/Notifications", "org.freedesktop.Notifications", QDBusConnection::sessionBus()),
    requestedFiles(),
    vCards(),
    requestedAccountsForPasswords(),
    prompt(0),
    currentConversation(0),
    restoreSelection(),
    needToRestore(false)
{
    m_ui->setupUi(this);
    m_ui->roster->setModel(&rosterModel);
    m_ui->roster->setContextMenuPolicy(Qt::CustomContextMenu);
    m_ui->roster->setColumnWidth(1, 30);
    m_ui->roster->setIconSize(QSize(20, 20));
    m_ui->roster->header()->setStretchLastSection(false);
    m_ui->roster->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    
    for (int i = static_cast<int>(Shared::AvailabilityLowest); i < static_cast<int>(Shared::AvailabilityHighest) + 1; ++i) {
        Shared::Availability av = static_cast<Shared::Availability>(i);
        m_ui->comboBox->addItem(Shared::availabilityIcon(av), Shared::Global::getName(av));
    }
    m_ui->comboBox->setCurrentIndex(static_cast<int>(Shared::Availability::offline));
    
    connect(m_ui->actionAccounts, &QAction::triggered, this, &Squawk::onAccounts);
    connect(m_ui->actionAddContact, &QAction::triggered, this, &Squawk::onNewContact);
    connect(m_ui->actionAddConference, &QAction::triggered, this, &Squawk::onNewConference);
    connect(m_ui->actionQuit, &QAction::triggered, this, &Squawk::close);
    connect(m_ui->comboBox, qOverload<int>(&QComboBox::activated), this, &Squawk::onComboboxActivated);
    //connect(m_ui->roster, &QTreeView::doubleClicked, this, &Squawk::onRosterItemDoubleClicked);
    connect(m_ui->roster, &QTreeView::customContextMenuRequested, this, &Squawk::onRosterContextMenu);
    connect(m_ui->roster, &QTreeView::collapsed, this, &Squawk::onItemCollepsed);
    connect(m_ui->roster->selectionModel(), &QItemSelectionModel::currentRowChanged, this, &Squawk::onRosterSelectionChanged);
    
    connect(rosterModel.accountsModel, &Models::Accounts::sizeChanged, this, &Squawk::onAccountsSizeChanged);
    connect(contextMenu, &QMenu::aboutToHide, this, &Squawk::onContextAboutToHide);
    //m_ui->mainToolBar->addWidget(m_ui->comboBox);
    
    if (testAttribute(Qt::WA_TranslucentBackground)) {
        m_ui->roster->viewport()->setAutoFillBackground(false);
    }
    
    QSettings settings;
    settings.beginGroup("ui");
    settings.beginGroup("window");
    if (settings.contains("geometry")) {
        restoreGeometry(settings.value("geometry").toByteArray());
    }
    if (settings.contains("state")) {
        restoreState(settings.value("state").toByteArray());
    }
    settings.endGroup();
    
    if (settings.contains("splitter")) {
        m_ui->splitter->restoreState(settings.value("splitter").toByteArray());
    }
    settings.endGroup();
}

Squawk::~Squawk() {
    delete contextMenu;
}

void Squawk::onAccounts()
{
    if (accounts == 0) {
        accounts = new Accounts(rosterModel.accountsModel);
        accounts->setAttribute(Qt::WA_DeleteOnClose);
        connect(accounts, &Accounts::destroyed, this, &Squawk::onAccountsClosed);
        connect(accounts, &Accounts::newAccount, this, &Squawk::newAccountRequest);
        connect(accounts, &Accounts::changeAccount, this, &Squawk::modifyAccountRequest);
        connect(accounts, &Accounts::connectAccount, this, &Squawk::connectAccount);
        connect(accounts, &Accounts::disconnectAccount, this, &Squawk::disconnectAccount);
        connect(accounts, &Accounts::removeAccount, this, &Squawk::removeAccountRequest);
        
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
        m_ui->actionAddConference->setEnabled(true);
    } else {
        m_ui->actionAddContact->setEnabled(false);
        m_ui->actionAddConference->setEnabled(false);
    }
}

void Squawk::onNewContact()
{
    NewContact* nc = new NewContact(rosterModel.accountsModel, this);
    
    connect(nc, &NewContact::accepted, this, &Squawk::onNewContactAccepted);
    connect(nc, &NewContact::rejected, nc, &NewContact::deleteLater);
    
    nc->exec();
}

void Squawk::onNewConference()
{
    JoinConference* jc = new JoinConference(rosterModel.accountsModel, this);
    
    connect(jc, &JoinConference::accepted, this, &Squawk::onJoinConferenceAccepted);
    connect(jc, &JoinConference::rejected, jc, &JoinConference::deleteLater);
    
    jc->exec();
}

void Squawk::onNewContactAccepted()
{
    NewContact* nc = static_cast<NewContact*>(sender());
    NewContact::Data value = nc->value();
    
    emit addContactRequest(value.account, value.jid, value.name, value.groups);
    
    nc->deleteLater();
}

void Squawk::onJoinConferenceAccepted()
{
    JoinConference* jc = static_cast<JoinConference*>(sender());
    JoinConference::Data value = jc->value();
    
    emit addRoomRequest(value.account, value.jid, value.nick, value.password, value.autoJoin);
    
    jc->deleteLater();
}

void Squawk::closeEvent(QCloseEvent* event)
{
    if (accounts != 0) {
        accounts->close();
    }
    
    for (Conversations::const_iterator itr = conversations.begin(), end = conversations.end(); itr != end; ++itr) {
        disconnect(itr->second, &Conversation::destroyed, this, &Squawk::onConversationClosed);
        itr->second->close();
    }
    conversations.clear();
    
    for (std::map<QString, VCard*>::const_iterator itr = vCards.begin(), end = vCards.end(); itr != end; ++itr) {
        disconnect(itr->second, &VCard::destroyed, this, &Squawk::onVCardClosed);
        itr->second->close();
    }
    vCards.clear();
    
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
    Shared::Availability av = Shared::Global::fromInt<Shared::Availability>(index);
    if (av != Shared::Availability::offline) {
        int size = rosterModel.accountsModel->rowCount(QModelIndex());
        if (size > 0) {
            emit changeState(av);
            for (int i = 0; i < size; ++i) {
                Models::Account* acc = rosterModel.accountsModel->getAccount(i);
                if (acc->getState() == Shared::ConnectionState::disconnected) {
                    emit connectAccount(acc->getName());
                }
            }
        } else {
            m_ui->comboBox->setCurrentIndex(static_cast<int>(Shared::Availability::offline));
        }
    } else {
        emit changeState(av);
        int size = rosterModel.accountsModel->rowCount(QModelIndex());
        for (int i = 0; i != size; ++i) {
            Models::Account* acc = rosterModel.accountsModel->getAccount(i);
            if (acc->getState() != Shared::ConnectionState::disconnected) {
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
        
    QSettings settings;
    settings.beginGroup("ui");
    settings.beginGroup("roster");
    settings.beginGroup(account);
    if (settings.value("expanded", false).toBool()) {
        QModelIndex ind = rosterModel.getAccountIndex(account);
        m_ui->roster->expand(ind);
    }
    settings.endGroup();
    settings.endGroup();
    settings.endGroup();
}

void Squawk::addGroup(const QString& account, const QString& name)
{
    rosterModel.addGroup(account, name);
    
    QSettings settings;
    settings.beginGroup("ui");
    settings.beginGroup("roster");
    settings.beginGroup(account);
    if (settings.value("expanded", false).toBool()) {
        QModelIndex ind = rosterModel.getAccountIndex(account);
        m_ui->roster->expand(ind);
        if (settings.value(name + "/expanded", false).toBool()) {
            m_ui->roster->expand(rosterModel.getGroupIndex(account, name));
        }
    }
    settings.endGroup();
    settings.endGroup();
    settings.endGroup();
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

void Squawk::stateChanged(Shared::Availability state)
{
    m_ui->comboBox->setCurrentIndex(static_cast<int>(state));
}

void Squawk::onRosterItemDoubleClicked(const QModelIndex& item)
{
    if (item.isValid()) {
        Models::Item* node = static_cast<Models::Item*>(item.internalPointer());
        if (node->type == Models::Item::reference) {
            node = static_cast<Models::Reference*>(node)->dereference();
        }
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
            Models::Account* acc = rosterModel.getAccount(id->account);
            Conversation* conv = 0;
            bool created = false;
            Models::Contact::Messages deque;
            if (itr != conversations.end()) {
                conv = itr->second;
            } else if (contact != 0) {
                created = true;
                conv = new Chat(acc, contact);
                contact->getMessages(deque);
            } else if (room != 0) {
                created = true;
                conv = new Room(acc, room);
                room->getMessages(deque);
                
                if (!room->getJoined()) {
                    emit setRoomJoined(id->account, id->name, true);
                }
            }
            
            if (conv != 0) {
                if (created) {
                    conv->setAttribute(Qt::WA_DeleteOnClose);
                    subscribeConversation(conv);
                    conversations.insert(std::make_pair(*id, conv));
                    
                    if (created) {
                        for (Models::Contact::Messages::const_iterator itr = deque.begin(), end = deque.end(); itr != end; ++itr) {
                            conv->addMessage(*itr);
                        }
                    }
                }
                
                conv->show();
                conv->raise();
                conv->activateWindow();
                
                if (res.size() > 0) {
                    conv->setPalResource(res);
                }
            }
            
            delete id;
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
    if (itr != conversations.end()) {
        conversations.erase(itr);
    }
    if (conv->isMuc) {
        Room* room = static_cast<Room*>(conv);
        if (!room->autoJoined()) {
            emit setRoomJoined(id.account, id.name, false);
        }
    }
}

void Squawk::onConversationDownloadFile(const QString& messageId, const QString& url)
{
    Conversation* conv = static_cast<Conversation*>(sender());
    std::map<QString, std::set<Models::Roster::ElId>>::iterator itr = requestedFiles.find(messageId);
    bool created = false;
    if (itr == requestedFiles.end()) {
        itr = requestedFiles.insert(std::make_pair(messageId, std::set<Models::Roster::ElId>())).first;
        created = true;
    }
    itr->second.insert(Models::Roster::ElId(conv->getAccount(), conv->getJid()));
    if (created) {
        emit downloadFileRequest(messageId, url);
    }
}

void Squawk::fileProgress(const QString& messageId, qreal value)
{
    std::map<QString, std::set<Models::Roster::ElId>>::const_iterator itr = requestedFiles.find(messageId);
    if (itr == requestedFiles.end()) {
        qDebug() << "fileProgress in UI Squawk but there is nobody waiting for that id" << messageId << ", skipping";
        return;
    } else {
        const std::set<Models::Roster::ElId>& convs = itr->second;
        for (std::set<Models::Roster::ElId>::const_iterator cItr = convs.begin(), cEnd = convs.end(); cItr != cEnd; ++cItr) {
            const Models::Roster::ElId& id = *cItr;
            Conversations::const_iterator c = conversations.find(id);
            if (c != conversations.end()) {
                c->second->responseFileProgress(messageId, value);
            }
            if (currentConversation != 0 && currentConversation->getId() == id) {
                currentConversation->responseFileProgress(messageId, value);
            }
        }
    }
}

void Squawk::fileError(const QString& messageId, const QString& error)
{
    std::map<QString, std::set<Models::Roster::ElId>>::const_iterator itr = requestedFiles.find(messageId);
    if (itr == requestedFiles.end()) {
        qDebug() << "fileError in UI Squawk but there is nobody waiting for that id" << messageId << ", skipping";
        return;
    } else {
        const std::set<Models::Roster::ElId>& convs = itr->second;
        for (std::set<Models::Roster::ElId>::const_iterator cItr = convs.begin(), cEnd = convs.end(); cItr != cEnd; ++cItr) {
            const Models::Roster::ElId& id = *cItr;
            Conversations::const_iterator c = conversations.find(id);
            if (c != conversations.end()) {
                c->second->fileError(messageId, error);
            }
            if (currentConversation != 0 && currentConversation->getId() == id) {
                currentConversation->fileError(messageId, error);
            }
        }
        requestedFiles.erase(itr);
    }
}

void Squawk::fileLocalPathResponse(const QString& messageId, const QString& path)
{
    std::map<QString, std::set<Models::Roster::ElId>>::const_iterator itr = requestedFiles.find(messageId);
    if (itr == requestedFiles.end()) {
        qDebug() << "fileLocalPathResponse in UI Squawk but there is nobody waiting for that path, skipping";
        return;
    } else {
        const std::set<Models::Roster::ElId>& convs = itr->second;
        for (std::set<Models::Roster::ElId>::const_iterator cItr = convs.begin(), cEnd = convs.end(); cItr != cEnd; ++cItr) {
            const Models::Roster::ElId& id = *cItr;
            Conversations::const_iterator c = conversations.find(id);
            if (c != conversations.end()) {
                c->second->responseLocalFile(messageId, path);
            }
            if (currentConversation != 0 && currentConversation->getId() == id) {
                currentConversation->responseLocalFile(messageId, path);
            }
        }
        
        requestedFiles.erase(itr);
    }
}

void Squawk::onConversationRequestLocalFile(const QString& messageId, const QString& url)
{
    Conversation* conv = static_cast<Conversation*>(sender());
    std::map<QString, std::set<Models::Roster::ElId>>::iterator itr = requestedFiles.find(messageId);
    bool created = false;
    if (itr == requestedFiles.end()) {
        itr = requestedFiles.insert(std::make_pair(messageId, std::set<Models::Roster::ElId>())).first;
        created = true;
    }
    itr->second.insert(Models::Roster::ElId(conv->getAccount(), conv->getJid()));
    if (created) {
        emit fileLocalPathRequest(messageId, url);
    }
}

void Squawk::accountMessage(const QString& account, const Shared::Message& data)
{
    const QString& from = data.getPenPalJid();
    Models::Roster::ElId id({account, from});
    Conversations::iterator itr = conversations.find(id);
    bool found = false;
    
    if (currentConversation != 0 && currentConversation->getId() == id) {
        currentConversation->addMessage(data);
        QApplication::alert(this);
        if (!isVisible() && !data.getForwarded()) {
            notify(account, data);
        }
        found = true;
    }
    
    if (itr != conversations.end()) {
        Conversation* conv = itr->second;
        conv->addMessage(data);
        QApplication::alert(conv);
        if (!found && conv->isMinimized()) {
            rosterModel.addMessage(account, data);
        }
        if (!conv->isVisible() && !data.getForwarded()) {
            notify(account, data);
        }
        found = true;
    }
    
    if (!found) {
        rosterModel.addMessage(account, data);
        if (!data.getForwarded()) {
            QApplication::alert(this);
            notify(account, data);
        }
    }
}

void Squawk::changeMessage(const QString& account, const QString& jid, const QString& id, const QMap<QString, QVariant>& data)
{
    Models::Roster::ElId eid({account, jid});
    bool found = false;
    
    if (currentConversation != 0 && currentConversation->getId() == eid) {
        currentConversation->changeMessage(id, data);
        QApplication::alert(this);
        found = true;
    }
    
    Conversations::iterator itr = conversations.find(eid);
    if (itr != conversations.end()) {
        Conversation* conv = itr->second;
        conv->changeMessage(id, data);
        if (!found && conv->isMinimized()) {
            rosterModel.changeMessage(account, jid, id, data);
        }
        found = true;
    } 
    
    if (!found) {
        rosterModel.changeMessage(account, jid, id, data);
    }
}

void Squawk::notify(const QString& account, const Shared::Message& msg)
{
    QString name = QString(rosterModel.getContactName(account, msg.getPenPalJid()));
    QString path = QString(rosterModel.getContactIconPath(account, msg.getPenPalJid(), msg.getPenPalResource()));
    QVariantList args;
    args << QString(QCoreApplication::applicationName());
    args << QVariant(QVariant::UInt);   //TODO some normal id
    if (path.size() > 0) {
        args << path;
    } else {
        args << QString("mail-message");    //TODO should here better be unknown user icon?
    }
    if (msg.getType() == Shared::Message::groupChat) {
        args << msg.getFromResource() + " from " + name;
    } else {
        args << name;
    }
    
    QString body(msg.getBody());
    QString oob(msg.getOutOfBandUrl());
    if (body == oob) {
        body = tr("Attached file");
    }
    
    args << body;
    args << QStringList();
    args << QVariantMap();
    args << 3000;
    dbus.callWithArgumentList(QDBus::AutoDetect, "Notify", args);
}

void Squawk::onConversationMessage(const Shared::Message& msg)
{
    Conversation* conv = static_cast<Conversation*>(sender());
    emit sendMessage(conv->getAccount(), msg);
    Models::Roster::ElId id = conv->getId();
    
    if (currentConversation != 0 && currentConversation->getId() == id) {
        if (conv == currentConversation) {
            Conversations::iterator itr = conversations.find(id);
            if (itr != conversations.end()) {
                itr->second->addMessage(msg);
            }
        } else {
            currentConversation->addMessage(msg);
        }
    }
}

void Squawk::onConversationMessage(const Shared::Message& msg, const QString& path)
{
    Conversation* conv = static_cast<Conversation*>(sender());
    Models::Roster::ElId id = conv->getId();
    std::map<QString, std::set<Models::Roster::ElId>>::iterator itr = requestedFiles.insert(std::make_pair(msg.getId(), std::set<Models::Roster::ElId>())).first;
    itr->second.insert(id);
    
    if (currentConversation != 0 && currentConversation->getId() == id) {
        if (conv == currentConversation) {
            Conversations::iterator itr = conversations.find(id);
            if (itr != conversations.end()) {
                itr->second->appendMessageWithUpload(msg, path);
            }
        } else {
            currentConversation->appendMessageWithUpload(msg, path);
        }
    }
    
    emit sendMessage(conv->getAccount(), msg, path);
}

void Squawk::onConversationRequestArchive(const QString& before)
{
    Conversation* conv = static_cast<Conversation*>(sender());
    requestArchive(conv->getAccount(), conv->getJid(), 20, before); //TODO amount as a settings value
}

void Squawk::responseArchive(const QString& account, const QString& jid, const std::list<Shared::Message>& list)
{
    Models::Roster::ElId id(account, jid);
    
    if (currentConversation != 0 && currentConversation->getId() == id) {
        currentConversation->responseArchive(list);
    }
    
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
            disconnect(conv, &Conversation::destroyed, this, &Squawk::onConversationClosed);
            disconnect(conv, &Conversation::requestArchive, this, &Squawk::onConversationRequestArchive);
            disconnect(conv, &Conversation::shown, this, &Squawk::onConversationShown);
            conv->close();
            conversations.erase(lItr);
        } else {
            ++itr;
        }
    }
    
    if (currentConversation != 0 && currentConversation->getAccount() == account) {
        currentConversation->deleteLater();
        currentConversation = 0;
        m_ui->filler->show();
    }
    
    rosterModel.removeAccount(account);
}

void Squawk::onRosterContextMenu(const QPoint& point)
{
    QModelIndex index = m_ui->roster->indexAt(point);
    if (index.isValid()) {
        Models::Item* item = static_cast<Models::Item*>(index.internalPointer());
        if (item->type == Models::Item::reference) {
            item = static_cast<Models::Reference*>(item)->dereference();
        }
        contextMenu->clear();
        bool hasMenu = false;
        bool active = item->getAccountConnectionState() == Shared::ConnectionState::connected;
        switch (item->type) {
            case Models::Item::account: {
                Models::Account* acc = static_cast<Models::Account*>(item);
                hasMenu = true;
                QString name = acc->getName();
                
                if (acc->getState() != Shared::ConnectionState::disconnected) {
                    QAction* con = contextMenu->addAction(Shared::icon("network-disconnect"), tr("Disconnect"));
                    con->setEnabled(active);
                    connect(con, &QAction::triggered, [this, name]() {
                        emit disconnectAccount(name);
                    });
                } else {
                    QAction* con = contextMenu->addAction(Shared::icon("network-connect"), tr("Connect"));
                    connect(con, &QAction::triggered, [this, name]() {
                        emit connectAccount(name);
                    });
                }
                
                QAction* card = contextMenu->addAction(Shared::icon("user-properties"), tr("VCard"));
                card->setEnabled(active);
                connect(card, &QAction::triggered, std::bind(&Squawk::onActivateVCard, this, name, acc->getBareJid(), true));
                
                QAction* remove = contextMenu->addAction(Shared::icon("edit-delete"), tr("Remove"));
                remove->setEnabled(active);
                connect(remove, &QAction::triggered, [this, name]() {
                    emit removeAccount(name);
                });
                
            }
                break;
            case Models::Item::contact: {
                Models::Contact* cnt = static_cast<Models::Contact*>(item);
                hasMenu = true;
                
                QAction* dialog = contextMenu->addAction(Shared::icon("mail-message"), tr("Open dialog"));
                dialog->setEnabled(active);
                connect(dialog, &QAction::triggered, [this, index]() {
                    onRosterItemDoubleClicked(index);
                });
                
                Shared::SubscriptionState state = cnt->getState();
                switch (state) {
                    case Shared::SubscriptionState::both:
                    case Shared::SubscriptionState::to: {
                        QAction* unsub = contextMenu->addAction(Shared::icon("news-unsubscribe"), tr("Unsubscribe"));
                        unsub->setEnabled(active);
                        connect(unsub, &QAction::triggered, [this, cnt]() {
                            emit unsubscribeContact(cnt->getAccountName(), cnt->getJid(), "");
                        });
                    }
                    break;
                    case Shared::SubscriptionState::from:
                    case Shared::SubscriptionState::unknown:
                    case Shared::SubscriptionState::none: {
                        QAction* sub = contextMenu->addAction(Shared::icon("news-subscribe"), tr("Subscribe"));
                        sub->setEnabled(active);
                        connect(sub, &QAction::triggered, [this, cnt]() {
                            emit subscribeContact(cnt->getAccountName(), cnt->getJid(), "");
                        });
                    }    
                }
                QString accName = cnt->getAccountName();
                QString cntJID = cnt->getJid();
                QString cntName = cnt->getName();
                
                QAction* rename = contextMenu->addAction(Shared::icon("edit-rename"), tr("Rename"));
                rename->setEnabled(active);
                connect(rename, &QAction::triggered, [this, cntName, accName, cntJID]() {
                    QInputDialog* dialog = new QInputDialog(this);
                    connect(dialog, &QDialog::accepted, [this, dialog, cntName, accName, cntJID]() {
                        QString newName = dialog->textValue();
                        if (newName != cntName) {
                            emit renameContactRequest(accName, cntJID, newName);
                        }
                        dialog->deleteLater();
                    });
                    connect(dialog, &QDialog::rejected, dialog, &QObject::deleteLater);
                    dialog->setInputMode(QInputDialog::TextInput);
                    dialog->setLabelText(tr("Input new name for %1\nor leave it empty for the contact \nto be displayed as %1").arg(cntJID));
                    dialog->setWindowTitle(tr("Renaming %1").arg(cntJID));
                    dialog->setTextValue(cntName);
                    dialog->exec();
                });
                
                
                QMenu* groupsMenu = contextMenu->addMenu(Shared::icon("group"), tr("Groups"));
                std::deque<QString> groupList = rosterModel.groupList(accName);
                for (QString groupName : groupList) {
                    QAction* gr = groupsMenu->addAction(groupName);
                    gr->setCheckable(true);
                    gr->setChecked(rosterModel.groupHasContact(accName, groupName, cntJID));
                    gr->setEnabled(active);
                    connect(gr, &QAction::toggled, [this, accName, groupName, cntJID](bool checked) {
                        if (checked) {
                            emit addContactToGroupRequest(accName, cntJID, groupName);
                        } else {
                            emit removeContactFromGroupRequest(accName, cntJID, groupName);
                        }
                    });
                }
                QAction* newGroup = groupsMenu->addAction(Shared::icon("group-new"), tr("New group"));
                newGroup->setEnabled(active);
                connect(newGroup, &QAction::triggered, [this, accName, cntJID]() {
                    QInputDialog* dialog = new QInputDialog(this);
                    connect(dialog, &QDialog::accepted, [this, dialog, accName, cntJID]() {
                        emit addContactToGroupRequest(accName, cntJID, dialog->textValue());
                        dialog->deleteLater();
                    });
                    connect(dialog, &QDialog::rejected, dialog, &QObject::deleteLater);
                    dialog->setInputMode(QInputDialog::TextInput);
                    dialog->setLabelText(tr("New group name"));
                    dialog->setWindowTitle(tr("Add %1 to a new group").arg(cntJID));
                    dialog->exec();
                });
                
                
                QAction* card = contextMenu->addAction(Shared::icon("user-properties"), tr("VCard"));
                card->setEnabled(active);
                connect(card, &QAction::triggered, std::bind(&Squawk::onActivateVCard, this, accName, cnt->getJid(), false));
                
                QAction* remove = contextMenu->addAction(Shared::icon("edit-delete"), tr("Remove"));
                remove->setEnabled(active);
                connect(remove, &QAction::triggered, [this, cnt]() {
                    emit removeContactRequest(cnt->getAccountName(), cnt->getJid());
                });
                
            }
                break;
            case Models::Item::room: {
                Models::Room* room = static_cast<Models::Room*>(item);
                hasMenu = true;
                
                QAction* dialog = contextMenu->addAction(Shared::icon("mail-message"), tr("Open conversation"));
                dialog->setEnabled(active);
                connect(dialog, &QAction::triggered, [this, index]() {
                    onRosterItemDoubleClicked(index);
                });
                
                
                Models::Roster::ElId id(room->getAccountName(), room->getJid());
                if (room->getAutoJoin()) {
                    QAction* unsub = contextMenu->addAction(Shared::icon("news-unsubscribe"), tr("Unsubscribe"));
                    unsub->setEnabled(active);
                    connect(unsub, &QAction::triggered, [this, id]() {
                        emit setRoomAutoJoin(id.account, id.name, false);
                        if (conversations.find(id) == conversations.end()
                            && (currentConversation == 0 || currentConversation->getId() != id)
                        ) {    //to leave the room if it's not opened in a conversation window
                            emit setRoomJoined(id.account, id.name, false);
                        }
                    });
                } else {
                    QAction* unsub = contextMenu->addAction(Shared::icon("news-subscribe"), tr("Subscribe"));
                    unsub->setEnabled(active);
                    connect(unsub, &QAction::triggered, [this, id]() {
                        emit setRoomAutoJoin(id.account, id.name, true);
                        if (conversations.find(id) == conversations.end()
                            && (currentConversation == 0 || currentConversation->getId() != id)
                        ) {    //to join the room if it's not already joined
                            emit setRoomJoined(id.account, id.name, true);
                        }
                    });
                }
                
                QAction* remove = contextMenu->addAction(Shared::icon("edit-delete"), tr("Remove"));
                remove->setEnabled(active);
                connect(remove, &QAction::triggered, [this, id]() {
                    emit removeRoomRequest(id.account, id.name);
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

void Squawk::addRoomParticipant(const QString& account, const QString& jid, const QString& name, const QMap<QString, QVariant>& data)
{
    rosterModel.addRoomParticipant(account, jid, name, data);
}

void Squawk::changeRoomParticipant(const QString& account, const QString& jid, const QString& name, const QMap<QString, QVariant>& data)
{
    rosterModel.changeRoomParticipant(account, jid, name, data);
}

void Squawk::removeRoomParticipant(const QString& account, const QString& jid, const QString& name)
{
    rosterModel.removeRoomParticipant(account, jid, name);
}

void Squawk::responseVCard(const QString& jid, const Shared::VCard& card)
{
    std::map<QString, VCard*>::const_iterator itr = vCards.find(jid);
    if (itr != vCards.end()) {
        itr->second->setVCard(card);
        itr->second->hideProgress();
    }
}

void Squawk::onVCardClosed()
{
    VCard* vCard = static_cast<VCard*>(sender());
    
    std::map<QString, VCard*>::const_iterator itr = vCards.find(vCard->getJid());
    if (itr == vCards.end()) {
        qDebug() << "VCard has been closed but can not be found among other opened vCards, application is most probably going to crash";
        return;
    }
    vCards.erase(itr);
}

void Squawk::onActivateVCard(const QString& account, const QString& jid, bool edition)
{
    std::map<QString, VCard*>::const_iterator itr = vCards.find(jid);
    VCard* card;
    Models::Contact::Messages deque;
    if (itr != vCards.end()) {
        card = itr->second;
    } else {
        card = new VCard(jid, edition);
        if (edition) {
            card->setWindowTitle(tr("%1 account card").arg(account));
        } else {
            card->setWindowTitle(tr("%1 contact card").arg(jid));
        }
        card->setAttribute(Qt::WA_DeleteOnClose);
        vCards.insert(std::make_pair(jid, card));
        
        connect(card, &VCard::destroyed, this, &Squawk::onVCardClosed);
        connect(card, &VCard::saveVCard, std::bind( &Squawk::onVCardSave, this, std::placeholders::_1, account));
    }
    
    card->show();
    card->raise();
    card->activateWindow();
    card->showProgress(tr("Downloading vCard"));
    
    emit requestVCard(account, jid);
}

void Squawk::onVCardSave(const Shared::VCard& card, const QString& account)
{
    VCard* widget = static_cast<VCard*>(sender());
    emit uploadVCard(account, card);
    
    widget->deleteLater();
}

void Squawk::readSettings()
{
    QSettings settings;
    settings.beginGroup("ui");
    
    if (settings.contains("availability")) {
        int avail = settings.value("availability").toInt();
        m_ui->comboBox->setCurrentIndex(avail);
        emit stateChanged(Shared::Global::fromInt<Shared::Availability>(avail));
        
        int size = settings.beginReadArray("connectedAccounts");
        for (int i = 0; i < size; ++i) {
            settings.setArrayIndex(i);
            emit connectAccount(settings.value("name").toString());     //TODO  this is actually not needed, stateChanged event already connects everything you have
        }                                                               //      need to fix that
        settings.endArray();
    }
    settings.endGroup();
}

void Squawk::writeSettings()
{
    QSettings settings;
    settings.beginGroup("ui");
    settings.beginGroup("window");
    settings.setValue("geometry", saveGeometry());
    settings.setValue("state", saveState());
    settings.endGroup();
    
    settings.setValue("splitter", m_ui->splitter->saveState());
    
    settings.setValue("availability", m_ui->comboBox->currentIndex());
    settings.beginWriteArray("connectedAccounts");
    int size = rosterModel.accountsModel->rowCount(QModelIndex());
    for (int i = 0; i < size; ++i) {
        Models::Account* acc = rosterModel.accountsModel->getAccount(i);
        if (acc->getState() != Shared::ConnectionState::disconnected) {
            settings.setArrayIndex(i);
            settings.setValue("name", acc->getName());
        }
    }
    settings.endArray();
    
    settings.remove("roster");
    settings.beginGroup("roster");
    for (int i = 0; i < size; ++i) {
        QModelIndex acc = rosterModel.index(i, 0, QModelIndex());
        Models::Account* account = rosterModel.accountsModel->getAccount(i);
        QString accName = account->getName();
        settings.beginGroup(accName);
        
        settings.setValue("expanded", m_ui->roster->isExpanded(acc));
        std::deque<QString> groups = rosterModel.groupList(accName);
        for (const QString& groupName : groups) {
            settings.beginGroup(groupName);
            QModelIndex gIndex = rosterModel.getGroupIndex(accName, groupName);
            settings.setValue("expanded", m_ui->roster->isExpanded(gIndex));
            settings.endGroup();
        }
        
        settings.endGroup();
    }
    settings.endGroup();
    settings.endGroup();
}

void Squawk::onItemCollepsed(const QModelIndex& index)
{
    QSettings settings;
    Models::Item* item = static_cast<Models::Item*>(index.internalPointer());
    switch (item->type) {
        case Models::Item::account:
            settings.setValue("ui/roster/" + item->getName() + "/expanded", false);
            break;
        case Models::Item::group: {
            QModelIndex accInd = rosterModel.parent(index);
            Models::Account* account = rosterModel.accountsModel->getAccount(accInd.row());
            settings.setValue("ui/roster/" + account->getName() + "/" + item->getName() + "/expanded", false);
        }
            break;
        default:
            break;
    }
}

void Squawk::requestPassword(const QString& account)
{
    requestedAccountsForPasswords.push_back(account);
    checkNextAccountForPassword();
}

void Squawk::checkNextAccountForPassword()
{
    if (prompt == 0 && requestedAccountsForPasswords.size() > 0) {
        prompt = new QInputDialog(this);
        QString accName = requestedAccountsForPasswords.front();
        connect(prompt, &QDialog::accepted, this, &Squawk::onPasswordPromptAccepted);
        connect(prompt, &QDialog::rejected, this, &Squawk::onPasswordPromptRejected);
        prompt->setInputMode(QInputDialog::TextInput);
        prompt->setTextEchoMode(QLineEdit::Password);
        prompt->setLabelText(tr("Input the password for account %1").arg(accName));
        prompt->setWindowTitle(tr("Password for account %1").arg(accName));
        prompt->setTextValue("");
        prompt->exec();
    }
}

void Squawk::onPasswordPromptAccepted()
{
    emit responsePassword(requestedAccountsForPasswords.front(), prompt->textValue());
    onPasswordPromptDone();
}

void Squawk::onPasswordPromptDone()
{
    prompt->deleteLater();
    prompt = 0;
    requestedAccountsForPasswords.pop_front();
    checkNextAccountForPassword();
}

void Squawk::onPasswordPromptRejected()
{
    //for now it's the same on reject and on accept, but one day I'm gonna make 
    //"Asking for the password again on the authentication failure" feature
    //and here I'll be able to break the circle of password requests
    emit responsePassword(requestedAccountsForPasswords.front(), prompt->textValue());
    onPasswordPromptDone();
}

void Squawk::subscribeConversation(Conversation* conv)
{
    connect(conv, &Conversation::destroyed, this, &Squawk::onConversationClosed);
    connect(conv, qOverload<const Shared::Message&>(&Conversation::sendMessage), this, qOverload<const Shared::Message&>(&Squawk::onConversationMessage));
    connect(conv, qOverload<const Shared::Message&, const QString&>(&Conversation::sendMessage), 
            this, qOverload<const Shared::Message&, const QString&>(&Squawk::onConversationMessage));
    connect(conv, &Conversation::requestArchive, this, &Squawk::onConversationRequestArchive);
    connect(conv, &Conversation::requestLocalFile, this, &Squawk::onConversationRequestLocalFile);
    connect(conv, &Conversation::downloadFile, this, &Squawk::onConversationDownloadFile);
    connect(conv, &Conversation::shown, this, &Squawk::onConversationShown);
}

void Squawk::onRosterSelectionChanged(const QModelIndex& current, const QModelIndex& previous)
{   
    if (restoreSelection.isValid() && restoreSelection == current) {
        restoreSelection = QModelIndex();
        return;
    }
    
    if (current.isValid()) {
        Models::Item* node = static_cast<Models::Item*>(current.internalPointer());
        if (node->type == Models::Item::reference) {
            node = static_cast<Models::Reference*>(node)->dereference();
        }
        Models::Contact* contact = 0;
        Models::Room* room = 0;
        QString res;
        Models::Roster::ElId* id = 0;
        bool hasContext = true;
        switch (node->type) {
            case Models::Item::contact:
                contact = static_cast<Models::Contact*>(node);
                id = new Models::Roster::ElId(contact->getAccountName(), contact->getJid());
                break;
            case Models::Item::presence:
                contact = static_cast<Models::Contact*>(node->parentItem());
                id = new Models::Roster::ElId(contact->getAccountName(), contact->getJid());
                res = node->getName();
                hasContext = false;
                break;
            case Models::Item::room:
                room = static_cast<Models::Room*>(node);
                id = new Models::Roster::ElId(room->getAccountName(), room->getJid());
                break;
            case Models::Item::participant:
                room = static_cast<Models::Room*>(node->parentItem());
                id = new Models::Roster::ElId(room->getAccountName(), room->getJid());
                hasContext = false;
                break;
            case Models::Item::group:
                hasContext = false;
            default:
                break;
        }
        
        if (hasContext && QGuiApplication::mouseButtons() & Qt::RightButton) {
            if (id != 0) {
                delete id;
            }
            needToRestore = true;
            restoreSelection = previous;
            return;
        }
        
        if (id != 0) {
            if (currentConversation != 0) {
                if (currentConversation->getId() == *id) {
                    if (contact != 0) {
                        currentConversation->setPalResource(res);
                    }
                    return;
                } else {
                    currentConversation->deleteLater();
                }
            } else {
                m_ui->filler->hide();
            }
            
            Models::Account* acc = rosterModel.getAccount(id->account);
            Models::Contact::Messages deque;
            if (contact != 0) {
                currentConversation = new Chat(acc, contact);
                contact->getMessages(deque);
            } else if (room != 0) {
                currentConversation = new Room(acc, room);
                room->getMessages(deque);
                
                if (!room->getJoined()) {
                    emit setRoomJoined(id->account, id->name, true);
                }
            }
            if (!testAttribute(Qt::WA_TranslucentBackground)) {
                currentConversation->setFeedFrames(true, false, true, true);
            }
            
            subscribeConversation(currentConversation);
            for (Models::Contact::Messages::const_iterator itr = deque.begin(), end = deque.end(); itr != end; ++itr) {
                currentConversation->addMessage(*itr);
            }
            
            if (res.size() > 0) {
                currentConversation->setPalResource(res);
            }
            
            m_ui->splitter->insertWidget(1, currentConversation);
            
            delete id;
        } else {
            if (currentConversation != 0) {
                currentConversation->deleteLater();
                currentConversation = 0;
                m_ui->filler->show();
            }
        }
    } else {
        if (currentConversation != 0) {
            currentConversation->deleteLater();
            currentConversation = 0;
            m_ui->filler->show();
        }
    }
}

void Squawk::onContextAboutToHide()
{
    if (needToRestore) {
        needToRestore = false;
        m_ui->roster->selectionModel()->setCurrentIndex(restoreSelection, QItemSelectionModel::ClearAndSelect);
    }
}
