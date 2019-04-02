#include "ui/squawk.h"
#include "core/squawk.h"
#include "signalcatcher.h"
#include <QtWidgets/QApplication>
#include <QtCore/QThread>
#include <QtCore/QObject>
#include <QSettings>

int main(int argc, char *argv[])
{
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
    QObject::connect(&w, SIGNAL(connectAccount(const QString&)), squawk, SLOT(connectAccount(const QString&)));
    QObject::connect(&w, SIGNAL(disconnectAccount(const QString&)), squawk, SLOT(disconnectAccount(const QString&)));
    
    QObject::connect(squawk, SIGNAL(newAccount(const QMap<QString, QVariant>&)), &w, SLOT(newAccount(const QMap<QString, QVariant>&)));
    QObject::connect(squawk, SIGNAL(accountConnectionStateChanged(const QString&, int)), &w, SLOT(accountConnectionStateChanged(const QString&, int)));
    
    coreThread->start();

    int result = app.exec();
    coreThread->wait(500);      //TODO hate doing that but settings for some reason don't get saved to the disk
    
    return result;
}

