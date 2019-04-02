#include "signalcatcher.h"
#include <signal.h>
#include <sys/socket.h>
#include <unistd.h>

int SignalCatcher::sigintFd[2] = {0,0};

SignalCatcher::SignalCatcher(QCoreApplication *p_app, QObject *parent):
    QObject(parent),
    app(p_app)
{
    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, sigintFd))
    {
        qFatal("Couldn't create INT socketpair");
    }
    
    if (setup_unix_signal_handlers() != 0) 
    {
        qFatal("Couldn't install unix handlers");
    }
    
    snInt = new QSocketNotifier(sigintFd[1], QSocketNotifier::Read, this);
    connect(snInt, SIGNAL(activated(int)), this, SLOT(handleSigInt()));
}

SignalCatcher::~SignalCatcher()
{}

void SignalCatcher::handleSigInt()
{
    snInt->setEnabled(false);
    char tmp;
    ::read(sigintFd[1], &tmp, sizeof(tmp));

    app->quit();

    snInt->setEnabled(true);
}

void SignalCatcher::intSignalHandler(int unused)
{
    char a = 1;
    ::write(sigintFd[0], &a, sizeof(a));
}

int SignalCatcher::setup_unix_signal_handlers()
{
    struct sigaction s_int;

    s_int.sa_handler = SignalCatcher::intSignalHandler;
    sigemptyset(&s_int.sa_mask);
    s_int.sa_flags = 0;
    s_int.sa_flags |= SA_RESTART;

    if (sigaction(SIGINT, &s_int, 0) > 0)
       return 1;

    return 0;
}
