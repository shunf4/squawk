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

#ifndef CHAT_H
#define CHAT_H

#include "conversation.h"
#include "../models/contact.h"

namespace Ui
{
class Chat;
}
class Chat : public Conversation
{
    Q_OBJECT
public:
    Chat(Models::Account* acc, Models::Contact* p_contact, QWidget* parent = 0);
    ~Chat();
    
    void addMessage(const Shared::Message & data) override;

protected slots:
    void onContactChanged(Models::Item* item, int row, int col);
    
protected:
    void setName(const QString & name) override;
    void handleSendMessage(const QString & text) override;
    
private:
    void updateState();
    
private:
    Models::Contact* contact;
};

#endif // CHAT_H
