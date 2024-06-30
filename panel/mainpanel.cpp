/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * qmpanel - a minimal Qt-based desktop panel
 *
 * Copyright: 2010-2011 Razor team
 *            2020-2024 John Lindgren
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
#include <LayerShellQt/window.h>
#include <NETWM>
#include <QApplication>
#include <QScreen>
#include <private/qtx11extras_p.h>
#include <private/qwayland-xdg-shell.h>
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
    mLayout.addWidget(new TaskBar(res, this));
    mLayout.addWidget(new StatusNotifier(this));
    mLayout.addWidget(new ClockLabel(this));

    mLayout.setStretch(2, 1); // stretch taskbar

    if (qApp->nativeInterface<QNativeInterface::QWaylandApplication>())
    {
        (void)winId(); // create native window
        auto layerShell = LayerShellQt::Window::get(windowHandle());
        layerShell->setMargins(QMargins());
        layerShell->setLayer(LayerShellQt::Window::Layer::LayerTop);
        layerShell->setAnchors(LayerShellQt::Window::Anchors(
            LayerShellQt::Window::Anchor::AnchorBottom |
            LayerShellQt::Window::Anchor::AnchorLeft |
            LayerShellQt::Window::Anchor::AnchorRight));
        layerShell->setKeyboardInteractivity(
            LayerShellQt::Window::KeyboardInteractivity::
                KeyboardInteractivityNone);
    }

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

    // Under Wayland (or XWayland), we pick our own screen, so we also
    // need to watch for new screens added.
    if (getenv("WAYLAND_DISPLAY"))
    {
        connect(qApp, &QApplication::screenAdded, this,
                &MainPanel::updateGeometryTriple);
    }
}

MainPanel::~MainPanel()
{
    // The base QObject/QWidget destructor will emit QObject::destroyed
    // on child objects including menus. Disconnect first so that our
    // signal handlers don't run in a partially destructed state.
    for (auto menu : mMenusRegistered)
        menu->disconnect(this);
}

void MainPanel::registerMenu(QMenu * menu)
{
    mMenusRegistered.insert(menu);

    connect(menu, &QMenu::aboutToShow, this, [this, menu]() {
        mMenusShown.insert(menu);
        updateKeyboardInteractivity();
        positionMenu(menu);
    });

    connect(menu, &QMenu::aboutToHide, this, [this, menu]() {
        mMenusShown.remove(menu);
        updateKeyboardInteractivity();
    });

    connect(menu, &QObject::destroyed, this, [this, menu]() {
        mMenusRegistered.remove(menu);
        mMenusShown.remove(menu);
        updateKeyboardInteractivity();
    });
}

void MainPanel::updateGeometry2(bool inShowEvent)
{
    QScreen * screen = QApplication::primaryScreen();
    QRect rect = screen->geometry();

    // Under Wayland (or XWayland), the primary screen may not be set.
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

        // layer-shell surfaces are tied to a specific screen once
        // shown. To change screens, we have to hide and reshow.
        // updateGeometry() will be called again from the showEvent().
        if (qApp->nativeInterface<QNativeInterface::QWaylandApplication>() &&
            !inShowEvent)
        {
            hide(), show();
            return;
        }
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

    if (qApp->nativeInterface<QNativeInterface::QWaylandApplication>())
    {
        auto layerShell = LayerShellQt::Window::get(windowHandle());
        layerShell->setExclusiveZone(height());
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

void MainPanel::updateKeyboardInteractivity()
{
    if (qApp->nativeInterface<QNativeInterface::QWaylandApplication>())
    {
        auto layerShell = LayerShellQt::Window::get(windowHandle());
        layerShell->setKeyboardInteractivity(
            mMenusShown.empty() ? LayerShellQt::Window::KeyboardInteractivity::
                                      KeyboardInteractivityNone
                                : LayerShellQt::Window::KeyboardInteractivity::
                                      KeyboardInteractivityExclusive);
        // Force a surface commit immediately, before the menu popup is
        // created and attempts to grab the keyboard. update() results
        // in a delayed commit and does not work here.
        repaint();
    }
}

void MainPanel::positionMenu(QMenu * menu)
{
    if (qApp->nativeInterface<QNativeInterface::QWaylandApplication>())
    {
        (void)menu->winId(); // create native window
        auto parent = menu->parentWidget();
        menu->windowHandle()->setProperty(
            "_q_waylandPopupAnchorRect",
            QRect(parent->mapTo(this, QPoint()), parent->size()));
        menu->windowHandle()->setProperty(
            "_q_waylandPopupAnchor",
            QVariant::fromValue(Qt::Edge::TopEdge | Qt::Edge::LeftEdge));
        menu->windowHandle()->setProperty(
            "_q_waylandPopupGravity",
            QVariant::fromValue(Qt::Edge::TopEdge | Qt::Edge::RightEdge));
        menu->windowHandle()->setProperty(
            "_q_waylandPopupConstraintAdjustment",
            QtWayland::xdg_positioner::constraint_adjustment_slide_x);
    }
}
