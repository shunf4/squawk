#include "ui/squawk.h"
#include "core/squawk.h"
#include <QtWidgets/QApplication>
#include <QtCore/QThread>
#include <QtCore/QObject>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    Squawk w;
    w.show();
    
    Core::Squawk* squawk = new Core::Squawk();
    QThread* coreThread = new QThread();
    squawk->moveToThread(coreThread);
    
    QObject::connect(coreThread, SIGNAL(finished()), squawk, SLOT(deleteLater()));
    QObject::connect(coreThread, SIGNAL(started()), squawk, SLOT(start()));
    
    QObject::connect(&w, SIGNAL(newAccountRequest(const QMap<QString, QVariant>&)), squawk, SLOT(newAccountRequest(const QMap<QString, QVariant>&)));
    QObject::connect(&w, SIGNAL(connectAccount(const QString&)), squawk, SLOT(connectAccount(const QString&)));
    QObject::connect(&w, SIGNAL(disconnectAccount(const QString&)), squawk, SLOT(disconnectAccount(const QString&)));
    
    QObject::connect(squawk, SIGNAL(newAccount(const QMap<QString, QVariant>&)), &w, SLOT(newAccount(const QMap<QString, QVariant>&)));
    QObject::connect(squawk, SIGNAL(accountConnectionStateChanged(const QString&, int)), &w, SLOT(accountConnectionStateChanged(const QString&, int)));
    
    //QObject::connect(this, &Controller::operate, worker, &Worker::doWork);
    //QObject::connect(worker, &Worker::resultReady, this, &Controller::handleResults);
    coreThread->start();

    return app.exec();
}

