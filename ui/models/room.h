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

#ifndef MODELS_ROOM_H
#define MODELS_ROOM_H

#include "item.h"
#include "participant.h"
#include "shared/enums.h"
#include "shared/message.h"

namespace Models {

/**
 * @todo write docs
 */
class Room : public Models::Item
{
    Q_OBJECT
public:
    typedef std::deque<Shared::Message> Messages;
    Room(const QString& p_jid, const QMap<QString, QVariant> &data, Item *parentItem = 0);
    ~Room();
    
    int columnCount() const override;
    QVariant data(int column) const override;
    
    unsigned int getUnreadMessagesCount() const;
    bool getJoined() const;
    bool getAutoJoin() const;
    QString getJid() const;
    QString getNick() const;
    QString getRoomName() const;
    QString getSubject() const;
    
    QIcon getStatusIcon(bool big = false) const;
    QString getStatusText() const;
    
    void setJoined(bool p_joined);
    void setAutoJoin(bool p_autoJoin);
    void setJid(const QString& p_jid);
    void setNick(const QString& p_nick);
    void setSubject(const QString& sub);
    
    void update(const QString& field, const QVariant& value);
    
    void addMessage(const Shared::Message& data);
    void changeMessage(const QString& id, const QMap<QString, QVariant>& data);
    unsigned int getMessagesCount() const;
    void dropMessages();
    void getMessages(Messages& container) const;
    
    void addParticipant(const QString& name, const QMap<QString, QVariant>& data);
    void changeParticipant(const QString& name, const QMap<QString, QVariant>& data);
    void removeParticipant(const QString& name);
    
    void toOfflineState() override;
    QString getDisplayedName() const override;
    Shared::Avatar getAvatarState() const;
    QString getAvatarPath() const;
    std::map<QString, const Participant&> getParticipants() const;
    QString getParticipantIconPath(const QString& name) const;
    
signals:
    void participantJoined(const Participant& participant);
    void participantLeft(const QString& name);
    
private:
    void handleParticipantUpdate(std::map<QString, Participant*>::const_iterator itr, const QMap<QString, QVariant>& data);
    
protected:
    bool columnInvolvedInDisplay(int col) override;
    void setAvatarState(Shared::Avatar p_state);
    void setAvatarState(unsigned int p_state);
    void setAvatarPath(const QString& path);
    
private:
    bool autoJoin;
    bool joined;
    QString jid;
    QString nick;
    QString subject;
    Shared::Avatar avatarState;
    QString avatarPath;
    Messages messages;
    std::map<QString, Participant*> participants;

};

}

#endif // MODELS_ROOM_H
