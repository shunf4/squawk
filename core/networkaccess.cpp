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


#include <QtWidgets/QApplication>
#include <QtCore/QDir>

#include "networkaccess.h"

Core::NetworkAccess::NetworkAccess(QObject* parent):
    QObject(parent),
    running(false),
    manager(0),
    storage("fileURLStorage"),
    downloads(),
    uploads()
{
}

Core::NetworkAccess::~NetworkAccess()
{
    stop();
}

void Core::NetworkAccess::downladFile(const QString& url)
{
    std::map<QString, Transfer*>::iterator itr = downloads.find(url);
    if (itr != downloads.end()) {
        qDebug() << "NetworkAccess received a request to download a file" << url << ", but the file is currently downloading, skipping";
    } else {
        try {
            std::pair<QString, std::list<Shared::MessageInfo>> p = storage.getPath(url);
            if (p.first.size() > 0) {
                QFileInfo info(p.first);
                if (info.exists() && info.isFile()) {
                    emit downloadFileComplete(p.second, p.first);
                } else {
                    startDownload(p.second, url);
                }
            } else {
                startDownload(p.second, url);
            }
        } catch (const Archive::NotFound& e) {
            qDebug() << "NetworkAccess received a request to download a file" << url << ", but there is now record of which message uses that file, downloading anyway";
            storage.addFile(url);
            startDownload(std::list<Shared::MessageInfo>(), url);
        } catch (const Archive::Unknown& e) {
            qDebug() << "Error requesting file path:" << e.what();
            emit loadFileError(std::list<Shared::MessageInfo>(), QString("Database error: ") + e.what(), false);
        }
    }
}

void Core::NetworkAccess::start()
{
    if (!running) {
        manager = new QNetworkAccessManager();
        storage.open();
        running = true;
    }
}

void Core::NetworkAccess::stop()
{
    if (running) {
        storage.close();
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
        emit loadFileProgress(dwn->messages, progress, false);
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
            emit loadFileError(dwn->messages, errorText, false);
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
    QNetworkReply* rpl = static_cast<QNetworkReply*>(sender());
    QString url = rpl->url().toString();
    std::map<QString, Transfer*>::const_iterator itr = downloads.find(url);
    if (itr == downloads.end()) {
        qDebug() << "an error downloading" << url << ": the request is done but there is no record of it being downloaded, ignoring";
    } else {
        Transfer* dwn = itr->second;
        if (dwn->success) {
            qDebug() << "download success for" << url;
            QStringList hops = url.split("/");
            QString fileName = hops.back();
            QString jid;
            if (dwn->messages.size() > 0) {
                jid = dwn->messages.front().jid;
            }
            QString path = prepareDirectory(jid);
            if (path.size() > 0) {
                path = checkFileName(fileName, path);
                
                QFile file(path);
                if (file.open(QIODevice::WriteOnly)) {
                    file.write(dwn->reply->readAll());
                    file.close();
                    storage.setPath(url, path);
                    qDebug() << "file" << path << "was successfully downloaded";
                } else {
                    qDebug() << "couldn't save file" << path;
                    path = QString();
                }
            }
            
            if (path.size() > 0) {
                emit downloadFileComplete(dwn->messages, path);
            } else {
                //TODO do I need to handle the failure here or it's already being handled in error?
                //emit loadFileError(dwn->messages, path, false);
            }
        }
        
        dwn->reply->deleteLater();
        delete dwn;
        downloads.erase(itr);
    }
}

void Core::NetworkAccess::startDownload(const std::list<Shared::MessageInfo>& msgs, const QString& url)
{
    Transfer* dwn = new Transfer({msgs, 0, 0, true, "", url, 0});
    QNetworkRequest req(url);
    dwn->reply = manager->get(req);
    connect(dwn->reply, &QNetworkReply::downloadProgress, this, &NetworkAccess::onDownloadProgress);
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    connect(dwn->reply, qOverload<QNetworkReply::NetworkError>(&QNetworkReply::errorOccurred), this, &NetworkAccess::onDownloadError);
#else
    connect(dwn->reply, qOverload<QNetworkReply::NetworkError>(&QNetworkReply::error), this, &NetworkAccess::onDownloadError);
#endif
    connect(dwn->reply, &QNetworkReply::finished, this, &NetworkAccess::onDownloadFinished);
    downloads.insert(std::make_pair(url, dwn));
    emit loadFileProgress(dwn->messages, 0, false);
}

void Core::NetworkAccess::onUploadError(QNetworkReply::NetworkError code)
{
    QNetworkReply* rpl = static_cast<QNetworkReply*>(sender());
    QString url = rpl->url().toString();
    std::map<QString, Transfer*>::const_iterator itr = uploads.find(url);
    if (itr == uploads.end()) {
        qDebug() << "an error uploading" << url << ": the request is reporting an error but there is no record of it being uploading, ignoring";
    } else {
        QString errorText = getErrorText(code);
        if (errorText.size() > 0) {
            itr->second->success = false;
            Transfer* upl = itr->second;
            emit loadFileError(upl->messages, errorText, true);
        }
        
        //TODO deletion?
    }
}

void Core::NetworkAccess::onUploadFinished()
{
    QNetworkReply* rpl = static_cast<QNetworkReply*>(sender());
    QString url = rpl->url().toString();
    std::map<QString, Transfer*>::const_iterator itr = uploads.find(url);
    if (itr == downloads.end()) {
        qDebug() << "an error uploading" << url << ": the request is done there is no record of it being uploading, ignoring";
    } else {
        Transfer* upl = itr->second;
        if (upl->success) {
            qDebug() << "upload success for" << url;
            
            storage.addFile(upl->messages, upl->url, upl->path);
            emit uploadFileComplete(upl->messages, upl->url);
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
        emit loadFileProgress(upl->messages, progress, true);
    }
}

QString Core::NetworkAccess::getFileRemoteUrl(const QString& path)
{
    QString p;
    
    try {
        QString p = storage.getUrl(path);
    } catch (const Archive::NotFound& err) {
        
    } catch (...) {
        throw;
    }
    
    return p;
}

void Core::NetworkAccess::uploadFile(const Shared::MessageInfo& info, const QString& path, const QUrl& put, const QUrl& get, const QMap<QString, QString> headers)
{
    QFile* file = new QFile(path);
    Transfer* upl = new Transfer({{info}, 0, 0, true, path, get.toString(), file});
    QNetworkRequest req(put);
    for (QMap<QString, QString>::const_iterator itr = headers.begin(), end = headers.end(); itr != end; itr++) {
        req.setRawHeader(itr.key().toUtf8(), itr.value().toUtf8());
    }
    if (file->open(QIODevice::ReadOnly)) {
        upl->reply = manager->put(req, file);
        
        connect(upl->reply, &QNetworkReply::uploadProgress, this, &NetworkAccess::onUploadProgress);
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
        connect(upl->reply, qOverload<QNetworkReply::NetworkError>(&QNetworkReply::errorOccurred), this, &NetworkAccess::onUploadError);
#else
        connect(upl->reply, qOverload<QNetworkReply::NetworkError>(&QNetworkReply::error), this, &NetworkAccess::onUploadError);
#endif
        connect(upl->reply, &QNetworkReply::finished, this, &NetworkAccess::onUploadFinished);
        uploads.insert(std::make_pair(put.toString(), upl));
        emit loadFileProgress(upl->messages, 0, true);
    } else {
        qDebug() << "couldn't upload file" << path;
        emit loadFileError(upl->messages, "Error opening file", true);
        delete file;
        delete upl;
    }
}

void Core::NetworkAccess::registerFile(const QString& url, const QString& account, const QString& jid, const QString& id)
{
    storage.addFile(url, account, jid, id);
    std::map<QString, Transfer*>::iterator itr = downloads.find(url);
    if (itr != downloads.end()) {
        itr->second->messages.emplace_back(account, jid, id);   //TODO notification is going to happen the next tick, is that okay?
    }
}

void Core::NetworkAccess::registerFile(const QString& url, const QString& path, const QString& account, const QString& jid, const QString& id)
{
    storage.addFile(url, path, account, jid, id);
}

bool Core::NetworkAccess::checkAndAddToUploading(const QString& acc, const QString& jid, const QString id, const QString path)
{
    for (const std::pair<const QString, Transfer*>& pair : uploads) {
        Transfer* info = pair.second;
        if (pair.second->path == path) {
            std::list<Shared::MessageInfo>& messages = info->messages;
            bool dup = false;
            for (const Shared::MessageInfo& info : messages) {
                if (info.account == acc && info.jid == jid && info.messageId == id) {
                    dup = true;
                    break;
                }
            }
            if (!dup) {
                info->messages.emplace_back(acc, jid, id);   //TODO notification is going to happen the next tick, is that okay?
                return true;
            }
        }
    }
    
    return false;
}

QString Core::NetworkAccess::prepareDirectory(const QString& jid)
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    path += "/" + QApplication::applicationName();
    if (jid.size() > 0) {
        path += "/" + jid;
    }
    QDir location(path);
    
    if (!location.exists()) {
        bool res = location.mkpath(path);
        if (!res) {
            return "";
        } else {
            return path;
        }
    }
    return path;
}

QString Core::NetworkAccess::checkFileName(const QString& name, const QString& path)
{
    QStringList parts = name.split(".");
    QString suffix("");
    QStringList::const_iterator sItr = parts.begin();
    QString realName = *sItr;
    ++sItr;
    for (QStringList::const_iterator sEnd = parts.end(); sItr != sEnd; ++sItr) {
        suffix += "." + (*sItr);
    }
    QString postfix("");
    QFileInfo proposedName(path + realName + suffix);
    int counter = 0;
    while (proposedName.exists()) {
        QString count = QString("(") + std::to_string(++counter).c_str() + ")";
        proposedName = QFileInfo(path + realName + count + suffix);
    }
    
    return proposedName.absoluteFilePath();
}

QString Core::NetworkAccess::addMessageAndCheckForPath(const QString& url, const QString& account, const QString& jid, const QString& id)
{
    return storage.addMessageAndCheckForPath(url, account, jid, id);
}
