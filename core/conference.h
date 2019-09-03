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

#ifndef CORE_CONFERENCE_H
#define CORE_CONFERENCE_H

#include "rosteritem.h"
#include <QXmppMucManager.h>

namespace Core
{

/**
 * @todo write docs
 */
class Conference : public RosterItem
{
    Q_OBJECT
public:
    Conference(const QString& p_jid, const QString& p_account, bool p_autoJoin, const QString& p_name, const QString& p_nick, QXmppMucRoom* p_room);
    ~Conference();
    
    QString getNick() const;
    QString getSubject() const;
    void setNick(const QString& p_nick);
    
    bool getJoined() const;
    void setJoined(bool p_joined);
    
    bool getAutoJoin();
    void setAutoJoin(bool p_autoJoin);
    
signals:
    void nickChanged(const QString& nick);
    void joinedChanged(bool joined);
    void autoJoinChanged(bool autoJoin);
    void subjectChanged(const QString& subject);
    void addParticipant(const QString& name, const QMap<QString, QVariant>& data);
    void changeParticipant(const QString& name, const QMap<QString, QVariant>& data);
    void removeParticipant(const QString& name);
    
private:
    QString nick;
    QXmppMucRoom* room;
    bool joined;
    bool autoJoin;
    
private slots:
    void onRoomJoined();
    void onRoomLeft();
    void onRoomNameChanged(const QString& p_name);
    void onRoomSubjectChanged(const QString& p_name);
    void onRoomNickNameChanged(const QString& p_nick);
    void onRoomError(const QXmppStanza::Error& err);
    void onRoomParticipantAdded(const QString& p_name);
    void onRoomParticipantChanged(const QString& p_name);
    void onRoomParticipantRemoved(const QString& p_name);
    
};

}

#endif // CORE_CONFERENCE_H
