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

#ifndef MESSAGELINE_H
#define MESSAGELINE_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QResizeEvent>
#include <QIcon>

#include "shared/message.h"
#include "message.h"
#include "progress.h"

class MessageLine : public QWidget
{
    Q_OBJECT
public:
    enum Position {
        beggining,
        middle,
        end,
        invalid
    };
    MessageLine(bool p_room, QWidget* parent = 0);
    ~MessageLine();
    
    Position message(const Shared::Message& msg, bool forceOutgoing = false);
    void setMyName(const QString& name);
    void setPalName(const QString& jid, const QString& name);
    QString firstMessageId() const;
    void showBusyIndicator();
    void hideBusyIndicator();
    void responseLocalFile(const QString& messageId, const QString& path);
    void fileError(const QString& messageId, const QString& error);
    void fileProgress(const QString& messageId, qreal progress);
    void appendMessageWithUpload(const Shared::Message& msg, const QString& path);
    void appendMessageWithUploadNoSiganl(const Shared::Message& msg, const QString& path);
    void removeMessage(const QString& messageId);
    void setMyAvatarPath(const QString& p_path);
    void setPalAvatar(const QString& jid, const QString& path);
    void dropPalAvatar(const QString& jid);
    void changeMessage(const QString& id, const QMap<QString, QVariant>& data);
    void setExPalAvatars(const std::map<QString, QString>& data);
    void movePalAvatarToEx(const QString& name);
    
signals:
    void resize(int amount);
    void downloadFile(const QString& messageId, const QString& url);
    void uploadFile(const Shared::Message& msg, const QString& path);
    void requestLocalFile(const QString& messageId, const QString& url);
    
protected:
    void resizeEvent(QResizeEvent * event) override;
    
protected:
    void onDownload();
    void onUpload();
    
private:
    struct Comparator {
        bool operator()(const Shared::Message& a, const Shared::Message& b) const {
            return a.getTime() < b.getTime();
        }
        bool operator()(const Shared::Message* a, const Shared::Message* b) const {
            return a->getTime() < b->getTime();
        }
    };
    typedef std::map<QDateTime, Message*> Order;
    typedef std::map<QString, Message*> Index;
    Index messageIndex;
    Order messageOrder;
    Index myMessages;
    std::map<QString, Index> palMessages;
    std::map<QString, QString> uploadPaths;
    std::map<QString, QString> palAvatars;
    std::map<QString, QString> exPalAvatars;
    QVBoxLayout* layout;
    
    QString myName;
    QString myAvatarPath;
    std::map<QString, QString> palNames;
    Index uploading;
    Index downloading;
    bool room;
    bool busyShown;
    Progress progress;
    int lastHeight;
};

#endif // MESSAGELINE_H
