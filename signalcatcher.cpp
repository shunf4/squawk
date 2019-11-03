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
    connect(snInt, &QSocketNotifier::activated, this, &SignalCatcher::handleSigInt);
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
