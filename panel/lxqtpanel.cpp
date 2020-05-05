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

#include "lxqtpanel.h"
#include "plugin.h"

#include "../plugin-mainmenu/mainmenu.h"
#include "../plugin-quicklaunch/quicklaunch.h"
#include "../plugin-taskbar/taskbar.h"
#include "../plugin-tray/lxqttrayplugin.h"
#include "../plugin-worldclock/lxqtworldclock.h"

#include <QApplication>
#include <QScreen>

#include <KWindowSystem/KWindowSystem>
#include <KWindowSystem/NETWM>

LXQtPanel::LXQtPanel() : mLayout(this)
{
    setAttribute(Qt::WA_AcceptDrops);
    setAttribute(Qt::WA_AlwaysShowToolTips);
    setAttribute(Qt::WA_X11NetWmWindowTypeDock);

    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowDoesNotAcceptFocus |
                   Qt::WindowStaysOnTopHint);

    mLayout.setMargin(0);
    mLayout.setSpacing(0);

    mLayout.addWidget(new MainMenuButton(this));
    mLayout.addWidget(new QuickLaunch(this));
    mLayout.addWidget(new TaskBar(this));
    mLayout.addWidget((new LXQtTrayPlugin(this))->widget());
    mLayout.addWidget((new LXQtWorldClock(this))->widget());

    mLayout.setStretch(2, 1); // stretch taskbar

    mLayout.insertSpacing(3, 6); /* TODO: scale with DPI */
    mLayout.insertSpacing(5, 6); /* TODO: scale with DPI */
    mLayout.insertSpacing(7, 6); /* TODO: scale with DPI */

    show();

    KWindowSystem::setOnDesktop(effectiveWinId(), NET::OnAllDesktops);

    connect(qApp, &QApplication::primaryScreenChanged, this,
            &LXQtPanel::updateGeometry);
}

void LXQtPanel::updateGeometry()
{
    QScreen * screen = QApplication::primaryScreen();
    if (mScreen != screen)
    {
        if (mScreen)
        {
            disconnect(mScreen, &QScreen::geometryChanged, this,
                       &LXQtPanel::updateGeometry);
            disconnect(mScreen, &QScreen::virtualGeometryChanged, this,
                       &LXQtPanel::updateGeometry);
        }

        mScreen = screen;
        connect(mScreen, &QScreen::geometryChanged, this,
                &LXQtPanel::updateGeometry);
        connect(mScreen, &QScreen::virtualGeometryChanged, this,
                &LXQtPanel::updateGeometry);
    }

    QRect rect = screen->geometry();
    rect.setTop(rect.bottom() + 1 - sizeHint().height());

    if (rect != geometry())
    {
        setFixedSize(rect.size());
        setGeometry(rect);
    }

    // virtualGeometry() usually matches the X11 screen (not monitor) size
    int screenBottom = screen->virtualGeometry().bottom();
    KWindowSystem::setExtendedStrut(effectiveWinId(),
                                    /* left   */ 0, 0, 0,
                                    /* right  */ 0, 0, 0,
                                    /* top    */ 0, 0, 0,
                                    /* bottom */ screenBottom + 1 - rect.top(),
                                    rect.left(), rect.right() + 1);
}

QRect LXQtPanel::calcPopupPos(QPoint const & absolutePos,
                              QSize const & windowSize) const
{
    QRect screen = QApplication::primaryScreen()->geometry();
    QRect pos(QPoint(absolutePos.x(), geometry().top() - windowSize.height()),
              windowSize);

    if (pos.right() > screen.right())
        pos.moveRight(screen.right());

    return pos;
}

QRect LXQtPanel::calcPopupPos(QWidget * widget, const QSize & windowSize) const
{
    return calcPopupPos(geometry().topLeft() + widget->geometry().topLeft(),
                        windowSize);
}
