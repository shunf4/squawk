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

#include "networkaccess.h"

Core::NetworkAccess::NetworkAccess(QObject* parent):
    QObject(parent),
    running(false),
    manager(),
    files("files"),
    downloads()
{
}

Core::NetworkAccess::~NetworkAccess()
{
    
}

void Core::NetworkAccess::fileLocalPathRequest(const QString& messageId, const QString& url)
{
    std::map<QString, Download*>::iterator itr = downloads.find(url);
    if (itr != downloads.end()) {
        Download* dwn = itr->second;
        std::set<QString>::const_iterator mItr = dwn->messages.find(messageId);
        if (mItr == dwn->messages.end()) {
            dwn->messages.insert(messageId);
        }
        emit downloadFileProgress(messageId, dwn->progress);
    } else {
        try {
            QString path = files.getRecord(url);
            QFileInfo info(path);
            if (info.exists() && info.isFile()) {
                emit fileLocalPathResponse(messageId, path);
            } else {
                files.removeRecord(url);
                emit fileLocalPathResponse(messageId, "");
            }
        } catch (Archive::NotFound e) {
            emit fileLocalPathResponse(messageId, "");
        } catch (Archive::Unknown e) {
            qDebug() << "Error requesting file path:" << e.what();
            emit fileLocalPathResponse(messageId, "");
        }
    }
}

void Core::NetworkAccess::downladFileRequest(const QString& messageId, const QString& url)
{
    std::map<QString, Download*>::iterator itr = downloads.find(url);
    if (itr != downloads.end()) {
        Download* dwn = itr->second;
        std::set<QString>::const_iterator mItr = dwn->messages.find(messageId);
        if (mItr == dwn->messages.end()) {
            dwn->messages.insert(messageId);
        }
        emit downloadFileProgress(messageId, dwn->progress);
    } else {
        try {
            QString path = files.getRecord(url);
            QFileInfo info(path);
            if (info.exists() && info.isFile()) {
                emit fileLocalPathResponse(messageId, path);
            } else {
                files.removeRecord(url);
                startDownload(messageId, url);
            }
        } catch (Archive::NotFound e) {
            startDownload(messageId, url);
        } catch (Archive::Unknown e) {
            qDebug() << "Error requesting file path:" << e.what();
            startDownload(messageId, url);
        }
    }
}

void Core::NetworkAccess::start()
{
    if (!running) {
        files.open();
        running = true;
    }
}

void Core::NetworkAccess::stop()
{
    if (running) {
        files.close();
        running = false;
    }
}

void Core::NetworkAccess::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    QNetworkReply* rpl = static_cast<QNetworkReply*>(sender());
    QString url = rpl->url().toString();
    std::map<QString, Download*>::const_iterator itr = downloads.find(url);
    if (itr == downloads.end()) {
        qDebug() << "an error downloading" << url << ": the request had some progress but seems like noone is waiting for it, skipping";
    } else {
        Download* dwn = itr->second;
        qreal received = bytesReceived;
        qreal total = bytesTotal;
        qreal progress = received/total;
        dwn->progress = progress;
        for (std::set<QString>::const_iterator mItr = dwn->messages.begin(), end = dwn->messages.end(); mItr != end; ++mItr) {
            emit downloadFileProgress(*mItr, progress);
        }
    }
}

void Core::NetworkAccess::onRequestError(QNetworkReply::NetworkError code)
{
    QNetworkReply* rpl = static_cast<QNetworkReply*>(sender());
    QString url = rpl->url().toString();
    std::map<QString, Download*>::const_iterator itr = downloads.find(url);
    if (itr == downloads.end()) {
        qDebug() << "an error downloading" << url << ": the request is reporting an error but seems like noone is waiting for it, skipping";
    } else {
        itr->second->success = false;
    }
}

void Core::NetworkAccess::onRequestFinished()
{
    QString path("");
    QNetworkReply* rpl = static_cast<QNetworkReply*>(sender());
    QString url = rpl->url().toString();
    std::map<QString, Download*>::const_iterator itr = downloads.find(url);
    if (itr == downloads.end()) {
        qDebug() << "an error downloading" << url << ": the request is done but seems like noone is waiting for it, skipping";
    } else {
        Download* dwn = itr->second;
        if (dwn->success) {
            qDebug() << "download success for" << url;
            QStringList hops = url.split("/");
            QString fileName = hops.back();
            QStringList parts = fileName.split(".");
            path = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
            QString suffix("");
            QString realName = parts.front();
            for (QStringList::const_iterator sItr = (parts.begin()++), sEnd = parts.end(); sItr != sEnd; ++sItr) {
                suffix += "." + (*sItr);
            }
            QString postfix("");
            QFileInfo proposedName(path + realName + postfix + suffix);
            int counter = 0;
            while (proposedName.exists()) {
                suffix = QString("(") + std::to_string(++counter).c_str() + ")";
                proposedName = QFileInfo(path + realName + postfix + suffix);
            }
            
            path = proposedName.absoluteFilePath();
            QFile file(path);
            if (file.open(QIODevice::WriteOnly)) {
                file.write(dwn->reply->readAll());
                file.close();
                files.addRecord(url, path);
                qDebug() << "file" << path << "was successfully downloaded";
            } else {
                qDebug() << "couldn't save file" << path;
                path = "";
            }
        }
        
        for (std::set<QString>::const_iterator mItr = dwn->messages.begin(), end = dwn->messages.end(); mItr != end; ++mItr) {
            emit fileLocalPathResponse(*mItr, path);
        }
        
        dwn->reply->deleteLater();
        delete dwn;
        downloads.erase(itr);
    }
}

void Core::NetworkAccess::startDownload(const QString& messageId, const QString& url)
{
    Download* dwn = new Download({{messageId}, 0, 0, true});
    QNetworkRequest req(url);
    dwn->reply = manager.get(req);
    connect(dwn->reply, SIGNAL(downloadProgress(qint64, qint64)), SLOT(onDownloadProgress(qint64, qint64)));
    connect(dwn->reply, SIGNAL(error(QNetworkReply::NetworkError)), SLOT(onRequestError(QNetworkReply::NetworkError)));
    connect(dwn->reply, SIGNAL(finished()), SLOT(onRequestFinished()));
    downloads.insert(std::make_pair(url, dwn));
    emit downloadFileProgress(messageId, 0);
}

