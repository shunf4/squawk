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

#include "storage.h"

namespace Core {

/**
 * @todo write docs
 */
class NetworkAccess : public QObject
{
    Q_OBJECT
    struct Transfer;
public:
    NetworkAccess(QObject* parent = nullptr);
    virtual ~NetworkAccess();
    
    void start();
    void stop();
    
signals:
    void fileLocalPathResponse(const QString& messageId, const QString& path);
    void downloadFileProgress(const QString& messageId, qreal value);
    void downloadFileError(const QString& messageId, const QString& path);
    void uploadFileProgress(const QString& messageId, qreal value);
    void uploadFileError(const QString& messageId, const QString& path);
    
public slots:
    void fileLocalPathRequest(const QString& messageId, const QString& url);
    void downladFileRequest(const QString& messageId, const QString& url);
    void uploadFileRequest(const QString& messageId, const QString& url, const QString& path);
    
private:
    void startDownload(const QString& messageId, const QString& url);
    void startUpload(const QString& messageId, const QString& url, const QString& path);
    QString getErrorText(QNetworkReply::NetworkError code);
    
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
    Storage files;
    std::map<QString, Transfer*> downloads;
    std::map<QString, Transfer*> uploads;
    
    struct Transfer {
        std::set<QString> messages;
        qreal progress;
        QNetworkReply* reply;
        bool success;
        QString path;
        QFile* file;
    };
};

}

#endif // CORE_NETWORKACCESS_H
