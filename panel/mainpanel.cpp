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

#include "mainpanel.h"

#include "clocklabel.h"
#include "mainmenu.h"
#include "quicklaunch.h"
#include "statusnotifier/statusnotifier.h"
#include "taskbar.h"

#include <KX11Extras>
#include <NETWM>
#include <QApplication>
#include <QScreen>
#include <private/qtx11extras_p.h>
#include <stdlib.h>

MainPanel::MainPanel(Resources & res) : mLayout(this)
{
    setAttribute(Qt::WA_AcceptDrops);
    setAttribute(Qt::WA_AlwaysShowToolTips);
    setAttribute(Qt::WA_X11NetWmWindowTypeDock);

    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowDoesNotAcceptFocus |
                   Qt::WindowStaysOnTopHint);

    mLayout.setContentsMargins(QMargins());
    mLayout.setSpacing(logicalDpiX() / 24);

    mLayout.addWidget(new MainMenuButton(res, this));
    mLayout.addWidget(new QuickLaunch(res, this));
    mLayout.addWidget(new TaskBar(this));
    mLayout.addWidget(new StatusNotifier(this));
    mLayout.addWidget(new ClockLabel(this));

    mLayout.setStretch(2, 1); // stretch taskbar

    show();

    if (QX11Info::isPlatformX11())
    {
        KX11Extras::setOnDesktop(effectiveWinId(), NET::OnAllDesktops);
        KX11Extras::setType(effectiveWinId(), NET::Dock);
    }

    mUpdateTimer.setInterval(500);
    mUpdateTimer.setSingleShot(true);

    connect(&mUpdateTimer, &QTimer::timeout, this, &MainPanel::updateGeometry);
    connect(qApp, &QApplication::primaryScreenChanged, this,
            &MainPanel::updateGeometryTriple);
}

void MainPanel::updateGeometry()
{
    QScreen * screen = QApplication::primaryScreen();
    QRect rect = screen->geometry();

    // Under XWayland, the primary screen may not be set.
    // As a workaround, pick the largest/leftmost screen.
    if (getenv("WAYLAND_DISPLAY"))
    {
        for (QScreen * testScreen : screen->virtualSiblings())
        {
            QRect testRect = testScreen->geometry();
            if (testRect.width() > rect.width() ||
                (testRect.width() == rect.width() &&
                 testRect.left() < rect.left()))
            {
                screen = testScreen;
                rect = testRect;
            }
        }
    }

    if (mScreen != screen)
    {
        if (mScreen)
        {
            disconnect(mScreen, &QScreen::geometryChanged, this,
                       &MainPanel::updateGeometryTriple);
            disconnect(mScreen, &QScreen::virtualGeometryChanged, this,
                       &MainPanel::updateGeometryTriple);
            disconnect(mScreen, &QObject::destroyed, this,
                       &MainPanel::updateGeometryTriple);
        }

        mScreen = screen;
        connect(mScreen, &QScreen::geometryChanged, this,
                &MainPanel::updateGeometryTriple);
        connect(mScreen, &QScreen::virtualGeometryChanged, this,
                &MainPanel::updateGeometryTriple);
        connect(mScreen, &QObject::destroyed, this,
                &MainPanel::updateGeometryTriple);
    }

    rect.setTop(rect.bottom() + 1 - sizeHint().height());
    if (rect != geometry())
    {
        setFixedSize(rect.size());
        setGeometry(rect);
    }

    if (QX11Info::isPlatformX11())
    {
        // virtualGeometry() usually matches the X11 screen (not monitor) size
        int screenBottom = screen->virtualGeometry().bottom();
        KX11Extras::setExtendedStrut(effectiveWinId(),
                                     /* left   */ 0, 0, 0,
                                     /* right  */ 0, 0, 0,
                                     /* top    */ 0, 0, 0,
                                     /* bottom */ screenBottom + 1 - rect.top(),
                                     rect.left(), rect.right());
        xcb_flush(QX11Info::connection());
    }

    if (mUpdateCount > 0)
    {
        mUpdateTimer.start();
        mUpdateCount--;
    }
}

void MainPanel::updateGeometryTriple()
{
    // Sometimes QScreen::virtualGeometry doesn't update immediately
    // under XWayland (due to missing RandR events). As a workaround,
    // check for changes again after 0.5s and after 1s.
    mUpdateCount = 2;
    updateGeometry();
}
