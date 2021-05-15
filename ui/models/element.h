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

#ifndef ELEMENT_H
#define ELEMENT_H

#include "item.h"

#include "ui/widgets/messageline/messagefeed.h"

namespace Models {
    
class Element : public Item
{
    Q_OBJECT
protected:
    Element(Type p_type, const Account* acc, const QString& p_jid, const QMap<QString, QVariant> &data, Item *parentItem = 0);
    ~Element();
    
public:
    QString getJid() const;
    Shared::Avatar getAvatarState() const;
    QString getAvatarPath() const;
    
    virtual void update(const QString& field, const QVariant& value);
    
    void addMessage(const Shared::Message& data);
    void changeMessage(const QString& id, const QMap<QString, QVariant>& data);
    unsigned int getMessagesCount() const;
    void responseArchive(const std::list<Shared::Message> list, bool last);
    bool isRoom() const;
    void fileProgress(const QString& messageId, qreal value, bool up);
    void fileError(const QString& messageId, const QString& error, bool up);
    void fileComplete(const QString& messageId, bool up);
    
signals:
    void requestArchive(const QString& before);
    void fileDownloadRequest(const QString& url);
    void unnoticedMessage(const QString& account, const Shared::Message& msg);
    void localPathInvalid(const QString& path);
    
protected:
    void setJid(const QString& p_jid);
    void setAvatarState(Shared::Avatar p_state);
    void setAvatarState(unsigned int p_state);
    void setAvatarPath(const QString& path);
    bool columnInvolvedInDisplay(int col) override;
    const Account* getParentAccount() const override;
    
protected slots:
    void onFeedUnreadMessagesCountChanged();
    void onFeedUnnoticedMessage(const Shared::Message& msg);
    
protected:
    QString jid;
    QString avatarPath;
    Shared::Avatar avatarState;
    
    const Account* account;

public:
    MessageFeed* feed;
};

}

#endif // ELEMENT_H
