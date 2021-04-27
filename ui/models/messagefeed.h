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

#ifndef MESSAGEFEED_H
#define MESSAGEFEED_H

#include <QAbstractListModel>
#include <QDateTime>
#include <QString>

#include <set>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/ranked_index.hpp>
#include <boost/multi_index/mem_fun.hpp>

#include <shared/message.h>
#include <shared/icons.h>


namespace Models {
    class Element;
    struct Attachment;
    
class MessageFeed : public QAbstractListModel
{
    Q_OBJECT
public:
    MessageFeed(const Element* rosterItem, QObject *parent = nullptr);
    ~MessageFeed();
    
    void addMessage(const Shared::Message& msg);
    void changeMessage(const QString& id, const QMap<QString, QVariant>& data);
    void removeMessage(const QString& id);
    
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    
    bool canFetchMore(const QModelIndex & parent) const override;
    void fetchMore(const QModelIndex & parent) override;
    QHash<int, QByteArray> roleNames() const override;
    
    void responseArchive(const std::list<Shared::Message> list, bool last);
    void downloadAttachment(const QString& messageId);
    void uploadAttachment(const QString& messageId);
    bool registerUpload(const QString& messageId);
    
    unsigned int unreadMessagesCount() const;
    void fileProgress(const QString& messageId, qreal value, bool up);
    void fileError(const QString& messageId, const QString& error, bool up);
    void fileComplete(const QString& messageId, bool up);
    
    void incrementObservers();
    void decrementObservers();
    
signals:
    void requestArchive(const QString& before);
    void requestStateChange(bool requesting);
    void fileDownloadRequest(const QString& url);
    void unreadMessagesCountChanged();
    void newMessage(const Shared::Message& msg);
    void unnoticedMessage(const Shared::Message& msg);
    
public:
    enum MessageRoles {
        Text = Qt::UserRole + 1,
        Sender,
        Date,
        DeliveryState,
        Correction,
        SentByMe,
        Avatar,
        Attach,
        Id,
        Error,
        Bulk
    };
    
protected:
    bool sentByMe(const Shared::Message& msg) const;
    Attachment fillAttach(const Shared::Message& msg) const;
    QModelIndex modelIndexById(const QString& id) const;
    QModelIndex modelIndexByTime(const QString& id, const QDateTime& time) const;
    std::set<MessageRoles> detectChanges(const Shared::Message& msg, const QMap<QString, QVariant>& data) const;
    
private:
    enum SyncState {
        incomplete,
        syncing,
        complete
    };
    
    //tags
    struct id {};
    struct time {};
    
    typedef boost::multi_index_container<
        Shared::Message*,
        boost::multi_index::indexed_by<
            boost::multi_index::ordered_unique<
                boost::multi_index::tag<id>,
                boost::multi_index::const_mem_fun<
                    Shared::Message,
                    QString,
                    &Shared::Message::getId
                >
            >,
            boost::multi_index::ranked_non_unique<
                boost::multi_index::tag<time>,
                boost::multi_index::const_mem_fun<
                    Shared::Message,
                    QDateTime,
                    &Shared::Message::getTime
                >,
                std::greater<QDateTime>
            >
        >
    > Storage; 
    
    typedef Storage::index<id>::type StorageById;
    typedef Storage::index<time>::type StorageByTime;
    Storage storage;
    StorageById& indexById;
    StorageByTime& indexByTime;
    
    const Element* rosterItem;
    SyncState syncState;
    
    typedef std::map<QString, qreal> Progress;
    Progress uploads;
    Progress downloads;
    
    std::set<QString>* unreadMessages;
    uint16_t observersAmount;
    
    static const QHash<int, QByteArray> roles;
};

enum AttachmentType {
    none,
    remote,
    local,
    downloading,
    uploading,
    errorDownload,
    errorUpload,
    ready
};

struct Attachment {
    AttachmentType state;
    qreal progress;
    QString localPath;
    QString remotePath;
};

struct FeedItem {
    QString id;
    QString text;
    QString sender;
    QString avatar;
    QString error;
    bool sentByMe;
    bool correction;
    QDateTime date;
    Shared::Message::State state;
    Attachment attach;
};
};

Q_DECLARE_METATYPE(Models::Attachment);
Q_DECLARE_METATYPE(Models::FeedItem);

#endif // MESSAGEFEED_H
