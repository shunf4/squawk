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
    manager(0),
    files("files"),
    downloads(),
    uploads()
{
}

Core::NetworkAccess::~NetworkAccess()
{
    stop();
}

void Core::NetworkAccess::fileLocalPathRequest(const QString& messageId, const QString& url)
{
    std::map<QString, Transfer*>::iterator itr = downloads.find(url);
    if (itr != downloads.end()) {
        Transfer* dwn = itr->second;
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
    std::map<QString, Transfer*>::iterator itr = downloads.find(url);
    if (itr != downloads.end()) {
        Transfer* dwn = itr->second;
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
            emit downloadFileError(messageId, QString("Database error: ") + e.what());
        }
    }
}

void Core::NetworkAccess::start()
{
    if (!running) {
        manager = new QNetworkAccessManager();
        files.open();
        running = true;
    }
}

void Core::NetworkAccess::stop()
{
    if (running) {
        files.close();
        manager->deleteLater();
        manager = 0;
        running = false;
        
        for (std::map<QString, Transfer*>::const_iterator itr = downloads.begin(), end = downloads.end(); itr != end; ++itr) {
            itr->second->success = false;
            itr->second->reply->abort();        //assuming it's gonna call onRequestFinished slot
        }
    }
}

void Core::NetworkAccess::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    QNetworkReply* rpl = static_cast<QNetworkReply*>(sender());
    QString url = rpl->url().toString();
    std::map<QString, Transfer*>::const_iterator itr = downloads.find(url);
    if (itr == downloads.end()) {
        qDebug() << "an error downloading" << url << ": the request had some progress but seems like no one is waiting for it, skipping";
    } else {
        Transfer* dwn = itr->second;
        qreal received = bytesReceived;
        qreal total = bytesTotal;
        qreal progress = received/total;
        dwn->progress = progress;
        for (std::set<QString>::const_iterator mItr = dwn->messages.begin(), end = dwn->messages.end(); mItr != end; ++mItr) {
            emit downloadFileProgress(*mItr, progress);
        }
    }
}

void Core::NetworkAccess::onDownloadError(QNetworkReply::NetworkError code)
{
    QNetworkReply* rpl = static_cast<QNetworkReply*>(sender());
    QString url = rpl->url().toString();
    std::map<QString, Transfer*>::const_iterator itr = downloads.find(url);
    if (itr == downloads.end()) {
        qDebug() << "an error downloading" << url << ": the request is reporting an error but seems like no one is waiting for it, skipping";
    } else {
        QString errorText = getErrorText(code);
        if (errorText.size() > 0) {
            itr->second->success = false;
            Transfer* dwn = itr->second;
            for (std::set<QString>::const_iterator mItr = dwn->messages.begin(), end = dwn->messages.end(); mItr != end; ++mItr) {
                emit downloadFileError(*mItr, errorText);
            }
        }
    }
}

QString Core::NetworkAccess::getErrorText(QNetworkReply::NetworkError code)
{
    QString errorText("");
    switch (code) {
        case QNetworkReply::NoError:
            //this never is supposed to happen
            break;

    // network layer errors [relating to the destination server] (1-99):
        case QNetworkReply::ConnectionRefusedError:
            errorText = "Connection refused";
            break;
        case QNetworkReply::RemoteHostClosedError:
            errorText = "Remote server closed the connection";
            break;
        case QNetworkReply::HostNotFoundError:
            errorText = "Remote host is not found";
            break;
        case QNetworkReply::TimeoutError:
            errorText = "Connection was closed because it timed out";
            break;
        case QNetworkReply::OperationCanceledError:
            //this means I closed it myself by abort() or close(), don't think I need to notify here
            break;
        case QNetworkReply::SslHandshakeFailedError:
            errorText = "Security error";           //TODO need to handle sslErrors signal to get a better description here
            break;
        case QNetworkReply::TemporaryNetworkFailureError:
            //this means the connection is lost by opened route, but it's going to be resumed, not sure I need to notify
            break;
        case QNetworkReply::NetworkSessionFailedError:
            errorText = "Outgoing connection problem";
            break;
        case QNetworkReply::BackgroundRequestNotAllowedError:
            errorText = "Background request is not allowed";
            break;
        case QNetworkReply::TooManyRedirectsError:
            errorText = "The request was  redirected too many times";
            break;
        case QNetworkReply::InsecureRedirectError:
            errorText = "The request was redirected to insecure connection";
            break;
        case QNetworkReply::UnknownNetworkError:
            errorText = "Unknown network error";
            break;

    // proxy errors (101-199):
        case QNetworkReply::ProxyConnectionRefusedError:
            errorText = "The connection to the proxy server was refused";
            break;
        case QNetworkReply::ProxyConnectionClosedError:
            errorText = "Proxy server closed the connection";
            break;
        case QNetworkReply::ProxyNotFoundError:
            errorText = "Proxy host was not found";
            break;
        case QNetworkReply::ProxyTimeoutError:
            errorText = "Connection to the proxy server was closed because it timed out";
            break;
        case QNetworkReply::ProxyAuthenticationRequiredError:
            errorText = "Couldn't connect to proxy server, authentication is required";
            break;
        case QNetworkReply::UnknownProxyError:
            errorText = "Unknown proxy error";
            break;

    // content errors (201-299):
        case QNetworkReply::ContentAccessDenied:
            errorText = "The access to file is denied";
            break;
        case QNetworkReply::ContentOperationNotPermittedError:
            errorText = "The operation over requesting file is not permitted";
            break;
        case QNetworkReply::ContentNotFoundError:
            errorText = "The file was not found";
            break;
        case QNetworkReply::AuthenticationRequiredError:
            errorText = "Couldn't access the file, authentication is required";
            break;
        case QNetworkReply::ContentReSendError:
            errorText = "Sending error, one more attempt will probably solve this problem";
            break;
        case QNetworkReply::ContentConflictError:
            errorText = "The request could not be completed due to a conflict with the current state of the resource";
            break;
        case QNetworkReply::ContentGoneError:
            errorText = "The requested resource is no longer available at the server";
            break;
        case QNetworkReply::UnknownContentError:
            errorText = "Unknown content error";
            break;

    // protocol errors
        case QNetworkReply::ProtocolUnknownError:
            errorText = "Unknown protocol error";
            break;
        case QNetworkReply::ProtocolInvalidOperationError:
            errorText = "Requested operation is not permitted in this protocol";
            break;
        case QNetworkReply::ProtocolFailure:
            errorText = "Low level protocol error";
            break;

    // Server side errors (401-499)
        case QNetworkReply::InternalServerError:
            errorText = "Internal server error";
            break;
        case QNetworkReply::OperationNotImplementedError:
            errorText = "Server doesn't support requested operation";
            break;
        case QNetworkReply::ServiceUnavailableError:
            errorText = "The server is not available for this operation right now";
            break;
        case QNetworkReply::UnknownServerError:
            errorText = "Unknown server error";
            break;
    }
    return errorText;
}


void Core::NetworkAccess::onDownloadFinished()
{
    QString path("");
    QNetworkReply* rpl = static_cast<QNetworkReply*>(sender());
    QString url = rpl->url().toString();
    std::map<QString, Transfer*>::const_iterator itr = downloads.find(url);
    if (itr == downloads.end()) {
        qDebug() << "an error downloading" << url << ": the request is done but seems like noone is waiting for it, skipping";
    } else {
        Transfer* dwn = itr->second;
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
    Transfer* dwn = new Transfer({{messageId}, 0, 0, true, "", url, 0});
    QNetworkRequest req(url);
    dwn->reply = manager->get(req);
    connect(dwn->reply, &QNetworkReply::downloadProgress, this, &NetworkAccess::onDownloadProgress);
    connect(dwn->reply, qOverload<QNetworkReply::NetworkError>(&QNetworkReply::error), this, &NetworkAccess::onDownloadError);
    connect(dwn->reply, &QNetworkReply::finished, this, &NetworkAccess::onDownloadFinished);
    downloads.insert(std::make_pair(url, dwn));
    emit downloadFileProgress(messageId, 0);
}

void Core::NetworkAccess::onUploadError(QNetworkReply::NetworkError code)
{
    QNetworkReply* rpl = static_cast<QNetworkReply*>(sender());
    QString url = rpl->url().toString();
    std::map<QString, Transfer*>::const_iterator itr = uploads.find(url);
    if (itr == uploads.end()) {
        qDebug() << "an error uploading" << url << ": the request is reporting an error but seems like noone is waiting for it, skipping";
    } else {
        QString errorText = getErrorText(code);
        if (errorText.size() > 0) {
            itr->second->success = false;
            Transfer* upl = itr->second;
            for (std::set<QString>::const_iterator mItr = upl->messages.begin(), end = upl->messages.end(); mItr != end; ++mItr) {
                emit uploadFileError(*mItr, errorText);
            }
        }
    }
}

void Core::NetworkAccess::onUploadFinished()
{
    QNetworkReply* rpl = static_cast<QNetworkReply*>(sender());
    QString url = rpl->url().toString();
    std::map<QString, Transfer*>::const_iterator itr = uploads.find(url);
    if (itr == downloads.end()) {
        qDebug() << "an error uploading" << url << ": the request is done but seems like no one is waiting for it, skipping";
    } else {
        Transfer* upl = itr->second;
        if (upl->success) {
            qDebug() << "upload success for" << url;
            files.addRecord(upl->url, upl->path);
        
            for (std::set<QString>::const_iterator mItr = upl->messages.begin(), end = upl->messages.end(); mItr != end; ++mItr) {
                emit fileLocalPathResponse(*mItr, upl->path);
                emit uploadFileComplete(*mItr, upl->url);
            }
        }
        
        upl->reply->deleteLater();
        upl->file->close();
        upl->file->deleteLater();
        delete upl;
        uploads.erase(itr);
    }
}

void Core::NetworkAccess::onUploadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    QNetworkReply* rpl = static_cast<QNetworkReply*>(sender());
    QString url = rpl->url().toString();
    std::map<QString, Transfer*>::const_iterator itr = uploads.find(url);
    if (itr == uploads.end()) {
        qDebug() << "an error downloading" << url << ": the request had some progress but seems like no one is waiting for it, skipping";
    } else {
        Transfer* upl = itr->second;
        qreal received = bytesReceived;
        qreal total = bytesTotal;
        qreal progress = received/total;
        upl->progress = progress;
        for (std::set<QString>::const_iterator mItr = upl->messages.begin(), end = upl->messages.end(); mItr != end; ++mItr) {
            emit uploadFileProgress(*mItr, progress);
        }
    }
}

void Core::NetworkAccess::startUpload(const QString& messageId, const QString& url, const QString& path)
{
    Transfer* upl = new Transfer({{messageId}, 0, 0, true, path, url, 0});
    QNetworkRequest req(url);
    QFile* file = new QFile(path);
    if (file->open(QIODevice::ReadOnly)) {
        upl->reply = manager->put(req, file);
        
        connect(upl->reply, &QNetworkReply::uploadProgress, this, &NetworkAccess::onUploadProgress);
        connect(upl->reply, qOverload<QNetworkReply::NetworkError>(&QNetworkReply::error), this, &NetworkAccess::onUploadError);
        connect(upl->reply, &QNetworkReply::finished, this, &NetworkAccess::onUploadFinished);
        uploads.insert(std::make_pair(url, upl));
        emit downloadFileProgress(messageId, 0);
    } else {
        qDebug() << "couldn't upload file" << path;
        emit uploadFileError(messageId, "Error opening file");
        delete file;
    }
}

void Core::NetworkAccess::uploadFileRequest(const QString& messageId, const QString& url, const QString& path)
{
    std::map<QString, Transfer*>::iterator itr = uploads.find(url);
    if (itr != uploads.end()) {
        Transfer* upl = itr->second;
        std::set<QString>::const_iterator mItr = upl->messages.find(messageId);
        if (mItr == upl->messages.end()) {
            upl->messages.insert(messageId);
        }
        emit uploadFileProgress(messageId, upl->progress);
    } else {
        try {
            QString ePath = files.getRecord(url);
            if (ePath == path) {
                QFileInfo info(path);
                if (info.exists() && info.isFile()) {
                    emit fileLocalPathResponse(messageId, path);
                } else {
                    files.removeRecord(url);
                    startUpload(messageId, url, path);
                }
            } else {
                QFileInfo info(path);
                if (info.exists() && info.isFile()) {
                    files.changeRecord(url, path);
                    emit fileLocalPathResponse(messageId, path);
                } else {
                    files.removeRecord(url);
                    startUpload(messageId, url, path);
                }
            }
        } catch (Archive::NotFound e) {
            startUpload(messageId, url, path);
        } catch (Archive::Unknown e) {
            qDebug() << "Error requesting file path on upload:" << e.what();
            emit uploadFileError(messageId, QString("Database error: ") + e.what());
        }
    }
}

QString Core::NetworkAccess::getFileRemoteUrl(const QString& path)
{
    return "";  //TODO this is a way not to upload some file more then 1 time, here I'm supposed to return that file GET url
}

bool Core::NetworkAccess::isUploading(const QString& path, const QString& messageId)
{
    return false; //TODO this is a way to avoid parallel uploading of the same files by different chats
                    //   message is is supposed to be added to the uploading messageids list 
                    //   the result should be true if there was an uploading file with this path
                    //   message id can be empty, then it's just to check and not to add
}

void Core::NetworkAccess::uploadFile(const QString& messageId, const QString& path, const QUrl& put, const QUrl& get, const QMap<QString, QString> headers)
{
    Transfer* upl = new Transfer({{messageId}, 0, 0, true, path, get.toString(), 0});
    QNetworkRequest req(put);
    for (QMap<QString, QString>::const_iterator itr = headers.begin(), end = headers.end(); itr != end; itr++) {
        req.setRawHeader(itr.key().toUtf8(), itr.value().toUtf8());
    }
    QFile* file = new QFile(path);
    if (file->open(QIODevice::ReadOnly)) {
        upl->reply = manager->put(req, file);
        
        connect(upl->reply, &QNetworkReply::uploadProgress, this, &NetworkAccess::onUploadProgress);
        connect(upl->reply, qOverload<QNetworkReply::NetworkError>(&QNetworkReply::error), this, &NetworkAccess::onUploadError);
        connect(upl->reply, &QNetworkReply::finished, this, &NetworkAccess::onUploadFinished);
        uploads.insert(std::make_pair(put.toString(), upl));
        emit downloadFileProgress(messageId, 0);
    } else {
        qDebug() << "couldn't upload file" << path;
        emit uploadFileError(messageId, "Error opening file");
        delete file;
    }
}
