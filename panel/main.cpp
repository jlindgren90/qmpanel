/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * qmpanel - a minimal Qt-based desktop panel
 *
 * Copyright: 2010-2011 Razor team
 *            2020 John Lindgren
 * Authors:
 *   Alexander Sokoloff <sokoloff.a@gmail.com>
 *   John Lindgren <john@jlindgren.net>
 *
 * This program or library is free software; you can redistribute it
 * and/or modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 *
 * END_COMMON_COPYRIGHT_HEADER */

#include <QApplication>
#include <signal.h>
#include <thread>

#include "mainpanel.h"
#include "resources.h"

sigset_t signal_set; // also used in resources.cpp

static void signal_thread()
{
    int signal;
    (void)sigwait(&signal_set, &signal);

    /* request qApp to exit cleanly */
    QMetaObject::invokeMethod(qApp, &QApplication::quit, Qt::QueuedConnection);
}

int main(int argc, char * argv[])
{
    /* block signals first */
    sigemptyset(&signal_set);
    sigaddset(&signal_set, SIGHUP);
    sigaddset(&signal_set, SIGINT);
    sigaddset(&signal_set, SIGTERM);
    sigprocmask(SIG_BLOCK, &signal_set, nullptr);

    QApplication app(argc, argv);
    app.setAttribute(Qt::AA_UseHighDpiPixmaps, true);

    /* monitor signals once qApp exists */
    std::thread(signal_thread).detach();

    Resources res;
    MainPanel panel(res);

    return app.exec();
}
