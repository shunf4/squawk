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

#include "global.h"

#include "enums.h"

Shared::Global* Shared::Global::instance = 0;
const std::set<QString> Shared::Global::supportedImagesExts = {"png", "jpg", "webp", "jpeg", "gif", "svg"};

#ifdef WITH_KIO
QLibrary Shared::Global::openFileManagerWindowJob("openFileManagerWindowJob");
Shared::Global::HighlightInFileManager Shared::Global::hfm = 0;
#endif

Shared::Global::Global():
    availability({
        tr("Online", "Availability"), 
        tr("Away", "Availability"), 
        tr("Absent", "Availability"), 
        tr("Busy", "Availability"), 
        tr("Chatty", "Availability"), 
        tr("Invisible", "Availability"), 
        tr("Offline", "Availability")
    }),
    connectionState({
        tr("Disconnected", "ConnectionState"), 
        tr("Connecting", "ConnectionState"), 
        tr("Connected", "ConnectionState"), 
        tr("Error", "ConnectionState")
    }),
    subscriptionState({
        tr("None", "SubscriptionState"), 
        tr("From", "SubscriptionState"), 
        tr("To", "SubscriptionState"), 
        tr("Both", "SubscriptionState"), 
        tr("Unknown", "SubscriptionState")
    }),
    affiliation({
        tr("Unspecified", "Affiliation"), 
        tr("Outcast", "Affiliation"), 
        tr("Nobody", "Affiliation"), 
        tr("Member", "Affiliation"), 
        tr("Admin", "Affiliation"), 
        tr("Owner", "Affiliation")
    }),
    role({
        tr("Unspecified", "Role"), 
        tr("Nobody", "Role"), 
        tr("Visitor", "Role"),
        tr("Participant", "Role"), 
        tr("Moderator", "Role")
    }),
    messageState({
        tr("Pending", "MessageState"), 
        tr("Sent", "MessageState"), 
        tr("Delivered", "MessageState"), 
        tr("Error", "MessageState")
    }),
    accountPassword({
        tr("Plain", "AccountPassword"),
        tr("Jammed", "AccountPassword"),
        tr("Always Ask", "AccountPassword"),
        tr("KWallet", "AccountPassword")
    }),
    accountPasswordDescription({
        tr("Your password is going to be stored in config file in plain text", "AccountPasswordDescription"),
        tr("Your password is going to be stored in config file but jammed with constant encryption key you can find in program source code. It might look like encryption but it's not", "AccountPasswordDescription"),
        tr("Squawk is going to query you for the password on every start of the program", "AccountPasswordDescription"),
        tr("Your password is going to be stored in KDE wallet storage (KWallet). You're going to be queried for permissions", "AccountPasswordDescription")
    }),
    pluginSupport({
        {"KWallet", false},
        {"openFileManagerWindowJob", false}
    }),
    fileCache()
{
    if (instance != 0) {
        throw 551;
    }
    
    instance = this;
    
#ifdef WITH_KIO
    openFileManagerWindowJob.load();
    if (openFileManagerWindowJob.isLoaded()) {
        hfm = (HighlightInFileManager) openFileManagerWindowJob.resolve("highlightInFileManager");
        if (hfm) {
            setSupported("openFileManagerWindowJob", true);
            qDebug() << "KIO::OpenFileManagerWindow support enabled";
        } else {
            qDebug() << "KIO::OpenFileManagerWindow support disabled: couldn't resolve required methods in the library";
        }
    } else {
        qDebug() << "KIO::OpenFileManagerWindow support disabled: couldn't load the library" << openFileManagerWindowJob.errorString();
    }
#endif
}


static const QSize defaultIconFileInfoHeight(50, 50);
Shared::Global::FileInfo Shared::Global::getFileInfo(const QString& path)
{
    std::map<QString, FileInfo>::const_iterator itr = instance->fileCache.find(path);
    if (itr == instance->fileCache.end()) {
        QMimeDatabase db;
        QMimeType type = db.mimeTypeForFile(path);
        QStringList parts = type.name().split("/");
        QString big = parts.front();
        QFileInfo info(path);
        
        FileInfo::Preview p = FileInfo::Preview::none;
        QSize size;
        if (big == "image") {
            if (parts.back() == "gif") {
                //TODO need to consider GIF as a movie
            }
            p = FileInfo::Preview::picture;
            QImage img(path);
            size = img.size();
        } else {
            size = defaultIconFileInfoHeight;
        }
        
        itr = instance->fileCache.insert(std::make_pair(path, FileInfo({info.fileName(), size, type, p}))).first;
    } 
    
    return itr->second;
}


Shared::Global * Shared::Global::getInstance()
{
    return instance;
}

QString Shared::Global::getName(Message::State rl)
{
    return instance->messageState[static_cast<int>(rl)];
}

QString Shared::Global::getName(Shared::Affiliation af)
{
    return instance->affiliation[static_cast<int>(af)];
}

QString Shared::Global::getName(Shared::Availability av)
{
    return instance->availability[static_cast<int>(av)];
}

QString Shared::Global::getName(Shared::ConnectionState cs)
{
    return instance->connectionState[static_cast<int>(cs)];
}

QString Shared::Global::getName(Shared::Role rl)
{
    return instance->role[static_cast<int>(rl)];
}

QString Shared::Global::getName(Shared::SubscriptionState ss)
{
    return instance->subscriptionState[static_cast<int>(ss)];
}

QString Shared::Global::getName(Shared::AccountPassword ap)
{
    return instance->accountPassword[static_cast<int>(ap)];
}

void Shared::Global::setSupported(const QString& pluginName, bool support)
{
    std::map<QString, bool>::iterator itr = instance->pluginSupport.find(pluginName);
    if (itr != instance->pluginSupport.end()) {
        itr->second = support;
    }
}

bool Shared::Global::supported(const QString& pluginName)
{
    std::map<QString, bool>::iterator itr = instance->pluginSupport.find(pluginName);
    if (itr != instance->pluginSupport.end()) {
        return itr->second;
    }
    return false;
}

QString Shared::Global::getDescription(Shared::AccountPassword ap)
{
    return instance->accountPasswordDescription[static_cast<int>(ap)];
}


static const QStringList query = {"query", "default", "inode/directory"};
static const QRegularExpression dolphinReg("[Dd]olphin");
static const QRegularExpression nautilusReg("[Nn]autilus");
static const QRegularExpression cajaReg("[Cc]aja");
static const QRegularExpression nemoReg("[Nn]emo");
static const QRegularExpression konquerorReg("kfmclient");
static const QRegularExpression pcmanfmQtReg("pcmanfm-qt");
static const QRegularExpression pcmanfmReg("pcmanfm");
static const QRegularExpression thunarReg("thunar");

void Shared::Global::highlightInFileManager(const QString& path)
{
#ifdef WITH_KIO
    if (supported("openFileManagerWindowJob")) {
        hfm(path);
        return;
    } else {
        qDebug() << "requested to highlight in file manager url" << path << "but it's not supported: KIO plugin isn't loaded, trying fallback";
    }
#else
    qDebug() << "requested to highlight in file manager url" << path << "but it's not supported: squawk wasn't compiled to support it, trying fallback";
#endif
    
    QFileInfo info = path;
    if (info.exists()) {
        QProcess proc;
        proc.start("xdg-mime", query);
        proc.waitForFinished();
        QString output = proc.readLine().simplified();
        
        QString folder;
        if (info.isDir()) {
            folder = info.canonicalFilePath();
        } else {
            folder = info.canonicalPath();
        }
        
        if (output.contains(dolphinReg)) {
            //there is a bug on current (21.04.0) dolphin, it works correct only if you already have dolphin launched
            proc.startDetached("dolphin", QStringList() << "--select" << info.canonicalFilePath());
            //KIO::highlightInFileManager({QUrl(info.canonicalFilePath())});
        } else if (output.contains(nautilusReg)) {
            proc.startDetached("nautilus", QStringList() << "--select" << info.canonicalFilePath());    //this worked on nautilus
        } else if (output.contains(cajaReg)) {
            proc.startDetached("caja", QStringList() << folder);  //caja doesn't seem to support file selection command line, gonna just open directory
        } else if (output.contains(nemoReg)) {
            proc.startDetached("nemo", QStringList() << info.canonicalFilePath());      //nemo supports selecting files without keys
        } else if (output.contains(konquerorReg)) {
            proc.startDetached("konqueror", QStringList() << "--select" << info.canonicalFilePath());   //this worked on konqueror
        } else if (output.contains(pcmanfmQtReg)) {
            proc.startDetached("pcmanfm-qt", QStringList() << folder);   //pcmanfm-qt doesn't seem to support open with selection, gonna just open directory
        } else if (output.contains(pcmanfmReg)) {
            proc.startDetached("pcmanfm", QStringList() << folder);   //pcmanfm also doesn't seem to support open with selection, gonna just open directory
        } else if (output.contains(thunarReg)) {
            proc.startDetached("thunar", QStringList() << folder);   //thunar doesn't seem to support open with selection, gonna just open directory
        } else {
            QDesktopServices::openUrl(QUrl::fromLocalFile(folder));
        }
    }
}


#define FROM_INT_INPL(Enum)                                                                 \
template<>                                                                                  \
Enum Shared::Global::fromInt(int src)                                                       \
{                                                                                           \
    if (src < static_cast<int>(Enum##Lowest) && src > static_cast<int>(Enum##Highest)) {    \
        throw EnumOutOfRange(#Enum);                                                        \
    }                                                                                       \
    return static_cast<Enum>(src);                                                          \
}                                                                                           \
template<>                                                                                  \
Enum Shared::Global::fromInt(unsigned int src) {return fromInt<Enum>(static_cast<int>(src));}

FROM_INT_INPL(Shared::Message::State)
FROM_INT_INPL(Shared::Affiliation)
FROM_INT_INPL(Shared::ConnectionState)
FROM_INT_INPL(Shared::Role)
FROM_INT_INPL(Shared::SubscriptionState)
FROM_INT_INPL(Shared::AccountPassword)
FROM_INT_INPL(Shared::Avatar)
FROM_INT_INPL(Shared::Availability)
