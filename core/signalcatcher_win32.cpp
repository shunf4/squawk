/*
 * Squawk messenger.
 * Copyright (C) 2021  Shunf4 <shun1048576@gmail.com>
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
#include <unistd.h>

SignalCatcher::SignalCatcher(QCoreApplication *p_app, QObject *parent):
    QObject(parent),
    app(p_app)
{
}

SignalCatcher::~SignalCatcher()
{}

void SignalCatcher::handleSigInt()
{
}

void SignalCatcher::intSignalHandler(int unused)
{
}

int SignalCatcher::setup_unix_signal_handlers()
{
    return 0;
}
