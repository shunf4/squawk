/*
 * Squawk messenger.
 * Copyright (C) 2019 Yury Gubich <blue@macaw.me>
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

#include "chat.h"

Chat::Chat(Models::Account* acc, Models::Contact* p_contact, QWidget* parent):
    Conversation(false, acc, p_contact->getJid(), "", parent),
    contact(p_contact)
{
    setName(p_contact->getContactName());
    updateState();
    setStatus(p_contact->getStatus());
    
    connect(contact, &Models::Contact::childChanged, this, &Chat::onContactChanged);
}

Chat::~Chat()
{
}

void Chat::onContactChanged(Models::Item* item, int row, int col)
{
    if (item == contact) {
        switch (col) {
            case 0:
                setName(contact->getContactName());
                break;
            case 3:
                updateState();
                break;
            case 5:
                setStatus(contact->getStatus());
                break;
        }
    }
}

void Chat::updateState()
{
    Shared::Availability av = contact->getAvailability();
    statusIcon->setPixmap(Shared::availabilityIcon(av, true).pixmap(40));
    statusIcon->setToolTip(QCoreApplication::translate("Global", Shared::availabilityNames[av].toLatin1()));
}

void Chat::handleSendMessage(const QString& text)
{
    Shared::Message msg(Shared::Message::chat);
    msg.setFrom(account->getFullJid());
    msg.setToJid(palJid);
    msg.setToResource(activePalResource);
    msg.setBody(text);
    msg.setOutgoing(true);
    msg.generateRandomId();
    msg.setCurrentTime();
    addMessage(msg);
    emit sendMessage(msg);
}

void Chat::addMessage(const Shared::Message& data)
{
    Conversation::addMessage(data);
    
    if (!data.getOutgoing()) {                          //TODO need to check if that was the last message
        const QString& res = data.getPenPalResource();
        if (res.size() > 0) {
            setPalResource(res);
        }
    }
}

void Chat::setName(const QString& name)
{
    Conversation::setName(name);
    line->setPalName(getJid(), name);
}

