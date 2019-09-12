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

#include "ui/squawk.h"
#include "core/squawk.h"
#include "signalcatcher.h"
#include <QtWidgets/QApplication>
#include <QtCore/QThread>
#include <QtCore/QObject>
#include <QSettings>

int main(int argc, char *argv[])
{
    qRegisterMetaType<Shared::Message>("Shared::Message");
    qRegisterMetaType<std::list<Shared::Message>>("std::list<Shared::Message>");
    qRegisterMetaType<QSet<QString>>("QSet<QString>");
    
    QApplication app(argc, argv);
    SignalCatcher sc(&app);
    
    QCoreApplication::setOrganizationName("Macaw");
    QCoreApplication::setOrganizationDomain("macaw.me");
    QCoreApplication::setApplicationName("Squawk");
    QCoreApplication::setApplicationVersion("0.0.3");
    
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
    
    Squawk w;
    w.show();
    
    Core::Squawk* squawk = new Core::Squawk();
    QThread* coreThread = new QThread();
    squawk->moveToThread(coreThread);
    
    QObject::connect(coreThread, SIGNAL(started()), squawk, SLOT(start()));
    QObject::connect(&app, SIGNAL(aboutToQuit()), squawk, SLOT(stop()));
    QObject::connect(squawk, SIGNAL(quit()), coreThread, SLOT(quit()));
    QObject::connect(coreThread, SIGNAL(finished()), squawk, SLOT(deleteLater()));
    
    QObject::connect(&w, SIGNAL(newAccountRequest(const QMap<QString, QVariant>&)), squawk, SLOT(newAccountRequest(const QMap<QString, QVariant>&)));
    QObject::connect(&w, SIGNAL(modifyAccountRequest(const QString&, const QMap<QString, QVariant>&)), 
                     squawk, SLOT(modifyAccountRequest(const QString&, const QMap<QString, QVariant>&)));
    QObject::connect(&w, SIGNAL(removeAccountRequest(const QString&)), squawk, SLOT(removeAccountRequest(const QString&)));
    QObject::connect(&w, SIGNAL(connectAccount(const QString&)), squawk, SLOT(connectAccount(const QString&)));
    QObject::connect(&w, SIGNAL(disconnectAccount(const QString&)), squawk, SLOT(disconnectAccount(const QString&)));
    QObject::connect(&w, SIGNAL(changeState(int)), squawk, SLOT(changeState(int)));
    QObject::connect(&w, SIGNAL(sendMessage(const QString&, const Shared::Message&)), squawk, SLOT(sendMessage(const QString&, const Shared::Message&)));
    QObject::connect(&w, SIGNAL(requestArchive(const QString&, const QString&, int, const QString&)), 
                     squawk, SLOT(requestArchive(const QString&, const QString&, int, const QString&)));
    QObject::connect(&w, SIGNAL(subscribeContact(const QString&, const QString&, const QString&)), 
                     squawk, SLOT(subscribeContact(const QString&, const QString&, const QString&)));
    QObject::connect(&w, SIGNAL(unsubscribeContact(const QString&, const QString&, const QString&)), 
                     squawk, SLOT(unsubscribeContact(const QString&, const QString&, const QString&)));
    QObject::connect(&w, SIGNAL(addContactRequest(const QString&, const QString&, const QString&, const QSet<QString>&)), 
                     squawk, SLOT(addContactRequest(const QString&, const QString&, const QString&, const QSet<QString>&)));
    QObject::connect(&w, SIGNAL(removeContactRequest(const QString&, const QString&)), 
                     squawk, SLOT(removeContactRequest(const QString&, const QString&)));
    QObject::connect(&w, SIGNAL(setRoomJoined(const QString&, const QString&, bool)), squawk, SLOT(setRoomJoined(const QString&, const QString&, bool)));
    QObject::connect(&w, SIGNAL(setRoomAutoJoin(const QString&, const QString&, bool)), squawk, SLOT(setRoomAutoJoin(const QString&, const QString&, bool)));
    
    QObject::connect(&w, SIGNAL(removeRoomRequest(const QString&, const QString&)), 
                     squawk, SLOT(removeRoomRequest(const QString&, const QString&)));
    QObject::connect(&w, SIGNAL(addRoomRequest(const QString&, const QString&, const QString&, const QString&, bool)), 
                     squawk, SLOT(addRoomRequest(const QString&, const QString&, const QString&, const QString&, bool)));
    QObject::connect(&w, SIGNAL(fileLocalPathRequest(const QString&, const QString&)), squawk, SLOT(fileLocalPathRequest(const QString&, const QString&)));
    
    QObject::connect(squawk, SIGNAL(newAccount(const QMap<QString, QVariant>&)), &w, SLOT(newAccount(const QMap<QString, QVariant>&)));
    QObject::connect(squawk, SIGNAL(addContact(const QString&, const QString&, const QString&, const QMap<QString, QVariant>&)), 
                     &w, SLOT(addContact(const QString&, const QString&, const QString&, const QMap<QString, QVariant>&)));
    QObject::connect(squawk, SIGNAL(changeAccount(const QString&, const QMap<QString, QVariant>&)), 
                     &w, SLOT(changeAccount(const QString&, const QMap<QString, QVariant>&)));
    QObject::connect(squawk, SIGNAL(removeAccount(const QString&)), &w, SLOT(removeAccount(const QString&)));
    QObject::connect(squawk, SIGNAL(addGroup(const QString&, const QString&)), &w, SLOT(addGroup(const QString&, const QString&)));
    QObject::connect(squawk, SIGNAL(removeGroup(const QString&, const QString&)), &w, SLOT(removeGroup(const QString&, const QString&)));
    QObject::connect(squawk, SIGNAL(removeContact(const QString&, const QString&)), &w, SLOT(removeContact(const QString&, const QString&)));
    QObject::connect(squawk, SIGNAL(removeContact(const QString&, const QString&, const QString&)), &w, SLOT(removeContact(const QString&, const QString&, const QString&)));
    QObject::connect(squawk, SIGNAL(changeContact(const QString&, const QString&, const QMap<QString, QVariant>&)), 
                     &w, SLOT(changeContact(const QString&, const QString&, const QMap<QString, QVariant>&)));
    QObject::connect(squawk, SIGNAL(addPresence(const QString&, const QString&, const QString&, const QMap<QString, QVariant>&)), 
                     &w, SLOT(addPresence(const QString&, const QString&, const QString&, const QMap<QString, QVariant>&)));
    QObject::connect(squawk, SIGNAL(removePresence(const QString&, const QString&, const QString&)), &w, SLOT(removePresence(const QString&, const QString&, const QString&)));
    QObject::connect(squawk, SIGNAL(stateChanged(int)), &w, SLOT(stateChanged(int)));
    QObject::connect(squawk, SIGNAL(accountMessage(const QString&, const Shared::Message&)), &w, SLOT(accountMessage(const QString&, const Shared::Message&)));
    QObject::connect(squawk, SIGNAL(responseArchive(const QString&, const QString&, const std::list<Shared::Message>&)), 
                     &w, SLOT(responseArchive(const QString&, const QString&, const std::list<Shared::Message>&)));
    
    QObject::connect(squawk, SIGNAL(addRoom(const QString&, const QString&, const QMap<QString, QVariant>&)), 
                     &w, SLOT(addRoom(const QString&, const QString&, const QMap<QString, QVariant>&)));
    QObject::connect(squawk, SIGNAL(changeRoom(const QString&, const QString&, const QMap<QString, QVariant>&)), 
                     &w, SLOT(changeRoom(const QString&, const QString&, const QMap<QString, QVariant>&)));
    QObject::connect(squawk, SIGNAL(removeRoom(const QString&, const QString&)), &w, SLOT(removeRoom(const QString&, const QString&)));
    QObject::connect(squawk, SIGNAL(addRoomParticipant(const QString&, const QString&, const QString&, const QMap<QString, QVariant>&)), 
                     &w, SLOT(addRoomParticipant(const QString&, const QString&, const QString&, const QMap<QString, QVariant>&)));
    QObject::connect(squawk, SIGNAL(changeRoomParticipant(const QString&, const QString&, const QString&, const QMap<QString, QVariant>&)), 
                     &w, SLOT(changeRoomParticipant(const QString&, const QString&, const QString&, const QMap<QString, QVariant>&)));
    QObject::connect(squawk, SIGNAL(removeRoomParticipant(const QString&, const QString&, const QString&)), 
                     &w, SLOT(removeRoomParticipant(const QString&, const QString&, const QString&)));
    QObject::connect(squawk, SIGNAL(fileLocalPathResponse(const QString&, const QString&)), &w, SLOT(fileLocalPathResponse(const QString&, const QString&)));
    
    
    //qDebug() << QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    
    coreThread->start();

    int result = app.exec();
    coreThread->wait(500);      //TODO hate doing that but settings for some reason don't get saved to the disk
    
    return result;
}

