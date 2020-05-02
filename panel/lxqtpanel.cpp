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

#include "../plugin-mainmenu/lxqtmainmenu.h"
#include "../plugin-quicklaunch/lxqtquicklaunch.h"
#include "../plugin-taskbar/lxqttaskbarplugin.h"
#include "../plugin-tray/lxqttrayplugin.h"
#include "../plugin-worldclock/lxqtworldclock.h"

#include <QApplication>
#include <QScreen>

#include <KWindowSystem/KWindowSystem>
#include <KWindowSystem/NETWM>

LXQtPanel::LXQtPanel(QWidget * parent) : QWidget(parent), mLayout(this)
{
    setAttribute(Qt::WA_AcceptDrops);
    setAttribute(Qt::WA_AlwaysShowToolTips);
    setAttribute(Qt::WA_X11NetWmWindowTypeDock);

    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowDoesNotAcceptFocus |
                   Qt::WindowStaysOnTopHint);

    mLayout.setMargin(0);
    mLayout.setSpacing(0);

    connect(qApp, &QApplication::primaryScreenChanged, this,
            &LXQtPanel::realign);

    loadPlugins();
    show();
}

void LXQtPanel::show()
{
    QWidget::show();
    KWindowSystem::setOnDesktop(effectiveWinId(), NET::OnAllDesktops);
}

void LXQtPanel::loadPlugins()
{
    auto addPlugin = [this](Plugin * plugin) {
        mLayout.addWidget(plugin->widget());
        connect(this, &LXQtPanel::realigned, plugin, &Plugin::realign);
    };

    addPlugin(new LXQtMainMenu(this));
    addPlugin(new LXQtQuickLaunch(this));
    addPlugin(new LXQtTaskBarPlugin(this));
    addPlugin(new LXQtTrayPlugin(this));
    addPlugin(new LXQtWorldClock(this));

    mLayout.setStretch(2, 1); // stretch taskbar

    mLayout.insertSpacing(3, 6); /* TODO: scale with DPI */
    mLayout.insertSpacing(5, 6); /* TODO: scale with DPI */
    mLayout.insertSpacing(7, 6); /* TODO: scale with DPI */
}

void LXQtPanel::setPanelGeometry()
{
    QScreen * screen = QApplication::primaryScreen();
    if (mScreen != screen)
    {
        if (mScreen)
        {
            disconnect(mScreen, &QScreen::geometryChanged, this,
                       &LXQtPanel::realign);
            disconnect(mScreen, &QScreen::virtualGeometryChanged, this,
                       &LXQtPanel::realign);
        }

        mScreen = screen;
        connect(mScreen, &QScreen::geometryChanged, this, &LXQtPanel::realign);
        connect(mScreen, &QScreen::virtualGeometryChanged, this,
                &LXQtPanel::realign);
    }

    QRect rect = screen->geometry();
    rect.setTop(rect.bottom() + 1 - sizeHint().height());

    if (rect != geometry())
    {
        setFixedSize(rect.size());
        setGeometry(rect);
    }
}

void LXQtPanel::realign()
{
    if (!isVisible())
        return;

    setPanelGeometry();
    updateWmStrut();
}

void LXQtPanel::updateWmStrut()
{
    WId wid = effectiveWinId();
    if (wid == 0 || !isVisible())
        return;

    // virtualGeometry() usually matches the X11 screen (not monitor) size
    const QRect wholeScreen = mScreen->virtualGeometry();
    const QRect rect = geometry();

    KWindowSystem::setExtendedStrut(wid, 0, 0, 0, // left
                                    0, 0, 0,      // right
                                    0, 0, 0,      // top
                                    wholeScreen.bottom() + 1 - rect.top(),
                                    rect.left(), rect.right() + 1); // bottom
}

bool LXQtPanel::event(QEvent * event)
{
    if (event->type() == QEvent::LayoutRequest)
        emit realigned();

    return QWidget::event(event);
}

void LXQtPanel::showEvent(QShowEvent * event)
{
    QWidget::showEvent(event);
    realign();
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
