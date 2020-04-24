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
#include "lxqtpanelapplication.h"
#include "popupmenu.h"
#include "plugin.h"

#include "../plugin-mainmenu/lxqtmainmenu.h"
#include "../plugin-quicklaunch/lxqtquicklaunchplugin.h"
#include "../plugin-taskbar/lxqttaskbarplugin.h"
#include "../plugin-tray/lxqttrayplugin.h"
#include "../plugin-worldclock/lxqtworldclock.h"

#include <QFileInfo>
#include <QScreen>
#include <QWindow>
#include <QX11Info>
#include <QDebug>
#include <QString>
#include <QDesktopWidget>
#include <QMenu>
#include <QMessageBox>
#include <QDropEvent>
#include <XdgIcon>
#include <XdgDirs>

#include <KWindowSystem/KWindowSystem>
#include <KWindowSystem/NETWM>

/************************************************

 ************************************************/
LXQtPanel::LXQtPanel(QWidget *parent) :
    QWidget(parent)
{
    //You can find information about the flags and widget attributes in your
    //Qt documentation or at https://doc.qt.io/qt-5/qt.html
    //Qt::FramelessWindowHint = Produces a borderless window. The user cannot
    //move or resize a borderless window via the window system. On X11, ...
    //Qt::WindowStaysOnTopHint = Informs the window system that the window
    //should stay on top of all other windows. Note that on ...
    Qt::WindowFlags flags = Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint;

    // NOTE: by PCMan:
    // In Qt 4, the window is not activated if it has Qt::WA_X11NetWmWindowTypeDock.
    // Since Qt 5, the default behaviour is changed. A window is always activated on mouse click.
    // Please see the source code of Qt5: src/plugins/platforms/xcb/qxcbwindow.cpp.
    // void QXcbWindow::handleButtonPressEvent(const xcb_button_press_event_t *event)
    // This new behaviour caused lxqt bug #161 - Cannot minimize windows from panel 1 when two task managers are open
    // Besides, this breaks minimizing or restoring windows when clicking on the taskbar buttons.
    // To workaround this regression bug, we need to add this window flag here.
    // However, since the panel gets no keyboard focus, this may decrease accessibility since
    // it's not possible to use the panel with keyboards. We need to find a better solution later.
    flags |= Qt::WindowDoesNotAcceptFocus;

    setWindowFlags(flags);
    //Adds _NET_WM_WINDOW_TYPE_DOCK to the window's _NET_WM_WINDOW_TYPE X11 window property. See https://standards.freedesktop.org/wm-spec/ for more details.
    setAttribute(Qt::WA_X11NetWmWindowTypeDock);
    //Enables tooltips for inactive windows.
    setAttribute(Qt::WA_AlwaysShowToolTips);
    //Allows data from drag and drop operations to be dropped onto the widget (see QWidget::setAcceptDrops()).
    setAttribute(Qt::WA_AcceptDrops);

    mLayout = new QHBoxLayout(this);
    mLayout->setMargin(0);
    mLayout->setSpacing(0);

    connect(qApp, &QGuiApplication::primaryScreenChanged, this, &LXQtPanel::realign);

    loadPlugins();

    show();
}

/************************************************

 ************************************************/
LXQtPanel::~LXQtPanel()
{
    mLayout->setEnabled(false);
}


/************************************************

 ************************************************/
void LXQtPanel::show()
{
    QWidget::show();
    KWindowSystem::setOnDesktop(effectiveWinId(), NET::OnAllDesktops);
}


/************************************************

 ************************************************/
void LXQtPanel::loadPlugins()
{
    mPlugins.append(new LXQtMainMenu(this));
    mPlugins.append(new LXQtQuickLaunchPlugin(this));
    mPlugins.append(new LXQtTaskBarPlugin(this));
    mPlugins.append(new LXQtTrayPlugin(this));
    mPlugins.append(new LXQtWorldClock(this));

    for (auto plugin : mPlugins)
    {
        mLayout->addWidget(plugin->widget());
        connect(this, &LXQtPanel::realigned, plugin, &Plugin::realign);
    }

    mLayout->setStretch(2, 1);
    mLayout->insertSpacing(3, 6); /* TODO: scale with DPI */
    mLayout->insertSpacing(5, 6); /* TODO: scale with DPI */
    mLayout->insertSpacing(7, 6); /* TODO: scale with DPI */
}

/************************************************

 ************************************************/
void LXQtPanel::setPanelGeometry()
{
    QScreen * screen = QApplication::primaryScreen();
    if (mScreen != screen)
    {
        if (mScreen)
        {
            disconnect(mScreen, &QScreen::geometryChanged, this, &LXQtPanel::realign);
            disconnect(mScreen, &QScreen::virtualGeometryChanged, this, &LXQtPanel::realign);
        }

        mScreen = screen;
        connect(mScreen, &QScreen::geometryChanged, this, &LXQtPanel::realign);
        connect(mScreen, &QScreen::virtualGeometryChanged, this, &LXQtPanel::realign);
    }

    const QRect currentScreen = screen->geometry();
    QRect rect;

    rect.setHeight(sizeHint().height());
    rect.setWidth(currentScreen.width());
    rect.moveLeft(currentScreen.left());
    rect.moveBottom(currentScreen.bottom());

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

    // Reserve our space on the screen ..........
    // It's possible that our geometry is not changed, but screen resolution is changed,
    // so resetting WM_STRUT is still needed. To make it simple, we always do it.
    updateWmStrut();
}


// Update the _NET_WM_PARTIAL_STRUT and _NET_WM_STRUT properties for the window
void LXQtPanel::updateWmStrut()
{
    WId wid = effectiveWinId();
    if(wid == 0 || !isVisible())
        return;

    const QRect wholeScreen = mScreen->virtualGeometry();
    const QRect rect = geometry();
    // NOTE: https://standards.freedesktop.org/wm-spec/wm-spec-latest.html
    // Quote from the EWMH spec: " Note that the strut is relative to the screen edge, and not the edge of the xinerama monitor."
    // So, we use the geometry of the whole screen to calculate the strut rather than using the geometry of individual monitors.
    // Though the spec only mention Xinerama and did not mention XRandR, the rule should still be applied.
    // At least openbox is implemented like this.
    KWindowSystem::setExtendedStrut(wid,
                                    /* Left   */  0, 0, 0,
                                    /* Right  */  0, 0, 0,
                                    /* Top    */  0, 0, 0,
                                    /* Bottom */  wholeScreen.bottom() - rect.bottom() + height(), rect.left(), rect.right()
                                   );
}


/************************************************

 ************************************************/
bool LXQtPanel::event(QEvent *event)
{
    switch (event->type())
    {
    case QEvent::LayoutRequest:
        emit realigned();
        break;

    default:
        break;
    }

    return QWidget::event(event);
}

/************************************************

 ************************************************/

void LXQtPanel::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    realign();
}

/************************************************

 ************************************************/
QRect LXQtPanel::calculatePopupWindowPos(QPoint const & absolutePos, QSize const & windowSize) const
{
    int x = absolutePos.x();
    int y = geometry().top() - windowSize.height();

    QRect res(QPoint(x, y), windowSize);

    QRect screen = QApplication::desktop()->screenGeometry(this);
    // NOTE: We cannot use AvailableGeometry() which returns the work area here because when in a
    // multihead setup with different resolutions. In this case, the size of the work area is limited
    // by the smallest monitor and may be much smaller than the current screen and we will place the
    // menu at the wrong place. This is very bad for UX. So let's use the full size of the screen.
    if (res.right() > screen.right())
        res.moveRight(screen.right());

    if (res.bottom() > screen.bottom())
        res.moveBottom(screen.bottom());

    if (res.left() < screen.left())
        res.moveLeft(screen.left());

    if (res.top() < screen.top())
        res.moveTop(screen.top());

    return res;
}

/************************************************

 ************************************************/
QRect LXQtPanel::calculatePopupWindowPos(QWidget *widget, const QSize &windowSize) const
{
    // Note: assuming there are not contentMargins around the "BackgroundWidget" (LXQtPanelWidget)
    return calculatePopupWindowPos(geometry().topLeft() + widget->geometry().topLeft(), windowSize);
}
