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

#include "../shared/global.h"
#include "../shared/messageinfo.h"
#include "../ui/squawk.h"
#include "signalcatcher.h"
#include "squawk.h"
#include <QLibraryInfo>
#include <QSettings>
#include <QStandardPaths>
#include <QTranslator>
#include <QtCore/QObject>
#include <QtCore/QThread>
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    qRegisterMetaType<Shared::Message>("Shared::Message");
    qRegisterMetaType<Shared::MessageInfo>("Shared::MessageInfo");
    qRegisterMetaType<Shared::VCard>("Shared::VCard");
    qRegisterMetaType<std::list<Shared::Message>>("std::list<Shared::Message>");
    qRegisterMetaType<std::list<Shared::MessageInfo>>("std::list<Shared::MessageInfo>");
    qRegisterMetaType<QSet<QString>>("QSet<QString>");
    qRegisterMetaType<Shared::ConnectionState>("Shared::ConnectionState");
    qRegisterMetaType<Shared::Availability>("Shared::Availability");
    
    QApplication app(argc, argv);
    SignalCatcher sc(&app);
#ifdef Q_OS_WIN
    // Windows need an organization name for QSettings to work
    // https://doc.qt.io/qt-5/qsettings.html#basic-usage
    {
        const QString& orgName = QApplication::organizationName();
        if (orgName.isNull() || orgName.isEmpty()) {
            QApplication::setOrganizationName("squawk");
        }
    }
#endif
    QApplication::setApplicationName("squawk");
    QApplication::setApplicationDisplayName("Squawk");
    QApplication::setApplicationVersion("0.1.5");
    
    QTranslator qtTranslator;
    qtTranslator.load("qt_" + QLocale::system().name(), QLibraryInfo::location(QLibraryInfo::TranslationsPath));
    app.installTranslator(&qtTranslator);
    
    QTranslator myappTranslator;
    QStringList shares = QStandardPaths::standardLocations(QStandardPaths::AppDataLocation);
    bool found = false;
    for (QString share : shares) {
        found = myappTranslator.load(QLocale(), QLatin1String("squawk"), ".", share + "/l10n");
        if (found) {
            break;
        }
    }
    if (!found) {
        myappTranslator.load(QLocale(), QLatin1String("squawk"), ".", QCoreApplication::applicationDirPath());
    }
    
    app.installTranslator(&myappTranslator);
    
    QIcon icon;
    icon.addFile(":images/logo.svg", QSize(16, 16));
    icon.addFile(":images/logo.svg", QSize(24, 24));
    icon.addFile(":images/logo.svg", QSize(32, 32));
    icon.addFile(":images/logo.svg", QSize(48, 48));
    icon.addFile(":images/logo.svg", QSize(64, 64));
    icon.addFile(":images/logo.svg", QSize(96, 96));
    icon.addFile(":images/logo.svg", QSize(128, 128));
    icon.addFile(":images/logo.svg", QSize(256, 256));
    icon.addFile(":images/logo.svg", QSize(512, 512));
    QApplication::setWindowIcon(icon);
    
    new Shared::Global();        //translates enums
    
    Squawk w;
    w.show();
    
    Core::Squawk* squawk = new Core::Squawk();
    QThread* coreThread = new QThread();
    squawk->moveToThread(coreThread);
    
    QObject::connect(coreThread, &QThread::started, squawk, &Core::Squawk::start);
    QObject::connect(&app, &QApplication::aboutToQuit, squawk, &Core::Squawk::stop);
    QObject::connect(&app, &QApplication::aboutToQuit, &w, &QMainWindow::close);
    QObject::connect(squawk, &Core::Squawk::quit, coreThread, &QThread::quit);
    QObject::connect(coreThread, &QThread::finished, squawk, &Core::Squawk::deleteLater);
    
    QObject::connect(&w, &Squawk::newAccountRequest, squawk, &Core::Squawk::newAccountRequest);
    QObject::connect(&w, &Squawk::modifyAccountRequest, squawk, &Core::Squawk::modifyAccountRequest);
    QObject::connect(&w, &Squawk::removeAccountRequest, squawk, &Core::Squawk::removeAccountRequest);
    QObject::connect(&w, &Squawk::connectAccount, squawk, &Core::Squawk::connectAccount);
    QObject::connect(&w, &Squawk::disconnectAccount, squawk, &Core::Squawk::disconnectAccount);
    QObject::connect(&w, &Squawk::changeState, squawk, &Core::Squawk::changeState);
    QObject::connect(&w, &Squawk::sendMessage, squawk,&Core::Squawk::sendMessage);
    QObject::connect(&w, &Squawk::resendMessage, squawk,&Core::Squawk::resendMessage);
    QObject::connect(&w, &Squawk::requestArchive, squawk, &Core::Squawk::requestArchive);
    QObject::connect(&w, &Squawk::subscribeContact, squawk, &Core::Squawk::subscribeContact);
    QObject::connect(&w, &Squawk::unsubscribeContact, squawk, &Core::Squawk::unsubscribeContact);
    QObject::connect(&w, &Squawk::addContactRequest, squawk, &Core::Squawk::addContactRequest);
    QObject::connect(&w, &Squawk::removeContactRequest, squawk, &Core::Squawk::removeContactRequest);
    QObject::connect(&w, &Squawk::setRoomJoined, squawk, &Core::Squawk::setRoomJoined);
    QObject::connect(&w, &Squawk::setRoomAutoJoin, squawk, &Core::Squawk::setRoomAutoJoin);
    QObject::connect(&w, &Squawk::removeRoomRequest, squawk, &Core::Squawk::removeRoomRequest);
    QObject::connect(&w, &Squawk::addRoomRequest, squawk, &Core::Squawk::addRoomRequest);
    QObject::connect(&w, &Squawk::fileDownloadRequest, squawk, &Core::Squawk::fileDownloadRequest);
    QObject::connect(&w, &Squawk::addContactToGroupRequest, squawk, &Core::Squawk::addContactToGroupRequest);
    QObject::connect(&w, &Squawk::removeContactFromGroupRequest, squawk, &Core::Squawk::removeContactFromGroupRequest);
    QObject::connect(&w, &Squawk::renameContactRequest, squawk, &Core::Squawk::renameContactRequest);
    QObject::connect(&w, &Squawk::requestVCard, squawk, &Core::Squawk::requestVCard);
    QObject::connect(&w, &Squawk::uploadVCard, squawk, &Core::Squawk::uploadVCard);
    QObject::connect(&w, &Squawk::responsePassword, squawk, &Core::Squawk::responsePassword);
    QObject::connect(&w, &Squawk::localPathInvalid, squawk, &Core::Squawk::onLocalPathInvalid);
    
    QObject::connect(squawk, &Core::Squawk::newAccount, &w, &Squawk::newAccount);
    QObject::connect(squawk, &Core::Squawk::addContact, &w, &Squawk::addContact);
    QObject::connect(squawk, &Core::Squawk::changeAccount, &w, &Squawk::changeAccount);
    QObject::connect(squawk, &Core::Squawk::removeAccount, &w, &Squawk::removeAccount);
    QObject::connect(squawk, &Core::Squawk::addGroup, &w, &Squawk::addGroup);
    QObject::connect(squawk, &Core::Squawk::removeGroup, &w, &Squawk::removeGroup);
    QObject::connect(squawk, qOverload<const QString&, const QString&>(&Core::Squawk::removeContact), 
                     &w, qOverload<const QString&, const QString&>(&Squawk::removeContact));
    QObject::connect(squawk, qOverload<const QString&, const QString&, const QString&>(&Core::Squawk::removeContact), 
                     &w, qOverload<const QString&, const QString&, const QString&>(&Squawk::removeContact));
    QObject::connect(squawk, &Core::Squawk::changeContact, &w, &Squawk::changeContact);
    QObject::connect(squawk, &Core::Squawk::addPresence, &w, &Squawk::addPresence);
    QObject::connect(squawk, &Core::Squawk::removePresence, &w, &Squawk::removePresence);
    QObject::connect(squawk, &Core::Squawk::stateChanged, &w, &Squawk::stateChanged);
    QObject::connect(squawk, &Core::Squawk::accountMessage, &w, &Squawk::accountMessage);
    QObject::connect(squawk, &Core::Squawk::changeMessage, &w, &Squawk::changeMessage);
    QObject::connect(squawk, &Core::Squawk::responseArchive, &w, &Squawk::responseArchive);
    QObject::connect(squawk, &Core::Squawk::addRoom, &w, &Squawk::addRoom);
    QObject::connect(squawk, &Core::Squawk::changeRoom, &w, &Squawk::changeRoom);
    QObject::connect(squawk, &Core::Squawk::removeRoom, &w, &Squawk::removeRoom);
    QObject::connect(squawk, &Core::Squawk::addRoomParticipant, &w, &Squawk::addRoomParticipant);
    QObject::connect(squawk, &Core::Squawk::changeRoomParticipant, &w, &Squawk::changeRoomParticipant);
    QObject::connect(squawk, &Core::Squawk::removeRoomParticipant, &w, &Squawk::removeRoomParticipant);
    QObject::connect(squawk, &Core::Squawk::fileDownloadComplete, &w, &Squawk::fileDownloadComplete);
    QObject::connect(squawk, &Core::Squawk::fileUploadComplete, &w, &Squawk::fileUploadComplete);
    QObject::connect(squawk, &Core::Squawk::fileProgress, &w, &Squawk::fileProgress);
    QObject::connect(squawk, &Core::Squawk::fileError, &w, &Squawk::fileError);
    QObject::connect(squawk, &Core::Squawk::responseVCard, &w, &Squawk::responseVCard);
    QObject::connect(squawk, &Core::Squawk::requestPassword, &w, &Squawk::requestPassword);
    QObject::connect(squawk, &Core::Squawk::ready, &w, &Squawk::readSettings);
    
    coreThread->start();

    int result = app.exec();
    
    w.writeSettings();
    coreThread->wait(500);      //TODO hate doing that but settings for some reason don't get saved to the disk
    
    
    return result;
}

