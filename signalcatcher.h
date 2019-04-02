#ifndef SIGNALCATCHER_H
#define SIGNALCATCHER_H

#include <QtCore/QCoreApplication>
#include <QtCore/QObject>
#include <QtCore/QSocketNotifier>

class SignalCatcher: public QObject
{
    Q_OBJECT
    
public:
    SignalCatcher(QCoreApplication *p_app, QObject *parent = 0);
    ~SignalCatcher();
    
    static void intSignalHandler(int unused);
    
public slots:
    void handleSigInt();
    
private:
    QCoreApplication *app;
    static int sigintFd[2];
    
    QSocketNotifier *snInt;
    
    static int setup_unix_signal_handlers();
};

#endif // SIGNALCATCHER_H
