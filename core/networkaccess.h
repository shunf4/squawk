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

#ifndef CORE_NETWORKACCESS_H
#define CORE_NETWORKACCESS_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QFileInfo>
#include <QFile>
#include <QStandardPaths>

#include <set>

#include "urlstorage.h"

namespace Core {

/**
 * @todo write docs
 */

//TODO Need to describe how to get rid of records when file is no longer reachable;
class NetworkAccess : public QObject
{
    Q_OBJECT
    struct Transfer;
public:
    NetworkAccess(QObject* parent = nullptr);
    virtual ~NetworkAccess();
    
    void start();
    void stop();
    
    QString getFileRemoteUrl(const QString& path);
    QString addMessageAndCheckForPath(const QString& url, const QString& account, const QString& jid, const QString& id);
    void uploadFile(const Shared::MessageInfo& info, const QString& path, const QUrl& put, const QUrl& get, const QMap<QString, QString> headers);
    bool checkAndAddToUploading(const QString& acc, const QString& jid, const QString id, const QString path);
    std::list<Shared::MessageInfo> reportPathInvalid(const QString& path);
    
signals:
    void loadFileProgress(const std::list<Shared::MessageInfo>& msgs, qreal value, bool up);
    void loadFileError(const std::list<Shared::MessageInfo>& msgs, const QString& text, bool up);
    void uploadFileComplete(const std::list<Shared::MessageInfo>& msgs, const QString& url);
    void downloadFileComplete(const std::list<Shared::MessageInfo>& msgs, const QString& path);
    
public slots:
    void downladFile(const QString& url);
    void registerFile(const QString& url, const QString& account, const QString& jid, const QString& id);
    void registerFile(const QString& url, const QString& path, const QString& account, const QString& jid, const QString& id);
    
private:
    void startDownload(const std::list<Shared::MessageInfo>& msgs, const QString& url);
    QString getErrorText(QNetworkReply::NetworkError code);
    QString prepareDirectory(const QString& jid);
    QString checkFileName(const QString& name, const QString& path);
    
private slots:
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void onDownloadError(QNetworkReply::NetworkError code);
    void onDownloadFinished();
    void onUploadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void onUploadError(QNetworkReply::NetworkError code);
    void onUploadFinished();
    
private:
    bool running;
    QNetworkAccessManager* manager;
    UrlStorage storage;
    std::map<QString, Transfer*> downloads;
    std::map<QString, Transfer*> uploads;
    
    struct Transfer {
        std::list<Shared::MessageInfo> messages;
        qreal progress;
        QNetworkReply* reply;
        bool success;
        QString path;
        QString url;
        QFile* file;
    };
};

}

#endif // CORE_NETWORKACCESS_H
