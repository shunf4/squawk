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
    QCoreApplication::setApplicationVersion("0.0.1");
    
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
    
    //qDebug() << QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    
    coreThread->start();

    int result = app.exec();
    coreThread->wait(500);      //TODO hate doing that but settings for some reason don't get saved to the disk
    
    return result;
}

