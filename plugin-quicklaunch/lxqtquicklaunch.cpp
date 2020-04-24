/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2010-2012 Razor team
 * Authors:
 *   Petr Vanek <petr@scribus.info>
 *   Kuzma Shapran <kuzma.shapran@gmail.com>
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

#include "lxqtquicklaunch.h"

#include <QDebug>
#include <QHBoxLayout>
#include <QToolButton>
#include <XdgAction>
#include <XdgDesktopFile>

LXQtQuickLaunch::LXQtQuickLaunch(QWidget * parent) : QWidget(parent)
{
    auto layout = new QHBoxLayout(this);
    layout->setMargin(0);
    layout->setSpacing(0);

    /* TODO: make this configurable */
    const QStringList apps = {
        "/usr/share/applications/nemo.desktop",
        "/usr/share/applications/xfce4-terminal.desktop",
        "/usr/share/applications/thunderbird.desktop",
        "/usr/share/applications/firefox.desktop",
        "/usr/share/applications/audacious.desktop",
        "/usr/share/applications/org.qt-project.qtcreator.desktop"};

    for (const QString & desktop : apps)
    {
        XdgDesktopFile xdg;
        if (!xdg.load(desktop))
        {
            qDebug() << "XdgDesktopFile" << desktop << "is not valid";
            continue;
        }
        if (!xdg.isSuitable())
        {
            qDebug() << "XdgDesktopFile" << desktop << "is not applicable";
            continue;
        }

        auto button = new QToolButton(this);
        button->setAutoRaise(true);
        button->setDefaultAction(new XdgAction(&xdg, this));
        layout->addWidget(button);
    }
}
