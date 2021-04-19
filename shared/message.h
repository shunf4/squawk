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

#ifndef SHAPER_MESSAGE_H
#define SHAPER_MESSAGE_H

#include <QString>
#include <QDateTime>
#include <QVariant>
#include <QMap>
#include <QDataStream>

namespace Shared {

/**
 * @todo write docs
 */
class Message {
public:
    enum Type {
        error,
        normal,
        chat,
        groupChat,
        headline
    };
    
    enum class State {
        pending,
        sent,
        delivered,
        error
    };
    static const State StateHighest = State::error;
    static const State StateLowest = State::pending;
    
    struct Change       //change functor, stores in idModified if ID has been modified during change
    {
        Change(const QMap<QString, QVariant>& _data);
        void operator() (Message& msg);
        void operator() (Message* msg);
        bool hasIdBeenModified() const;
        
    private:
        const QMap<QString, QVariant>& data;
        bool idModified;
    };
    
    Message(Type p_type);
    Message();
    
    void setFrom(const QString& from);
    void setFromResource(const QString& from);
    void setFromJid(const QString& from);
    void setTo(const QString& to);
    void setToResource(const QString& to);
    void setToJid(const QString& to);
    void setTime(const QDateTime& p_time);
    void setId(const QString& p_id);
    void setBody(const QString& p_body);
    void setThread(const QString& p_body);
    void setOutgoing(bool og);
    void setForwarded(bool fwd);
    void setType(Type t);
    void setCurrentTime();
    void setOutOfBandUrl(const QString& url);
    void setState(State p_state);
    void setEdited(bool p_edited);
    void setErrorText(const QString& err);
    bool change(const QMap<QString, QVariant>& data);
    void setStanzaId(const QString& sid);
    void setAttachPath(const QString& path);
    
    QString getFrom() const;
    QString getFromJid() const;
    QString getFromResource() const;
    QString getTo() const;
    QString getToJid() const;
    QString getToResource() const;
    QDateTime getTime() const;
    QString getId() const;
    QString getBody() const;
    QString getThread() const;
    bool getOutgoing() const;
    bool getForwarded() const;
    Type getType() const;
    bool hasOutOfBandUrl() const;
    bool storable() const;
    QString getOutOfBandUrl() const;
    State getState() const;
    bool getEdited() const;
    QString getErrorText() const;
    
    QString getPenPalJid() const;
    QString getPenPalResource() const;
    void generateRandomId();
    bool serverStored() const;
    QDateTime getLastModified() const;
    QString getOriginalBody() const;
    QString getStanzaId() const;
    QString getAttachPath() const;
    
    void serialize(QDataStream& data) const;
    void deserialize(QDataStream& data);
    
private:
    QString jFrom;
    QString rFrom;
    QString jTo;
    QString rTo;
    QString id;
    QString body;
    QDateTime time;
    QString thread;
    Type type;
    bool outgoing;
    bool forwarded;
    QString oob;
    State state;
    bool edited;
    QString errorText;
    QString originalMessage;
    QDateTime lastModified;
    QString stanzaId;
    QString attachPath;
};

}

#endif // SHAPER_MESSAGE_H
