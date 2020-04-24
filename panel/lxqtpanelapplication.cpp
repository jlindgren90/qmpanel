/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2010-2011 Razor team
 * Authors:
 *   Alexander Sokoloff <sokoloff.a@gmail.com>
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


#include "lxqtpanelapplication.h"
#include "lxqtpanel.h"
#include <LXQt/Settings>
#include <QtDebug>
#include <QUuid>
#include <QScreen>
#include <QWindow>
#include <QCommandLineParser>

LXQtPanelApplication::LXQtPanelApplication(int& argc, char** argv)
    : LXQt::Application(argc, argv, true)

{
    QCoreApplication::setApplicationName(QLatin1String("lxqt-panel"));
    const QString VERINFO = QStringLiteral(LXQT_PANEL_VERSION
                                           "\nliblxqt   " LXQT_VERSION
                                           "\nQt        " QT_VERSION_STR);

    QCoreApplication::setApplicationVersion(VERINFO);

    QCommandLineParser parser;
    parser.setApplicationDescription(QLatin1String("LXQt Panel"));
    parser.addHelpOption();
    parser.addVersionOption();
    parser.process(*this);

    mPanel = new LXQtPanel;
}

LXQtPanelApplication::~LXQtPanelApplication()
{
    delete mPanel;
}
