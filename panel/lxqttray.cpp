/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2011 Razor team
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

#include <QApplication>
#include <QDebug>
#include <QHBoxLayout>
#include <QStyle>
#include <QTimer>
#include <QX11Info>
#include <algorithm>
#include <vector>
#include "trayicon.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xrender.h>
#include <X11/extensions/Xdamage.h>
#include <xcb/xcb.h>
#include <xcb/damage.h>

#include "lxqttray.h"

#define _NET_SYSTEM_TRAY_ORIENTATION_HORZ 0
#define SYSTEM_TRAY_REQUEST_DOCK 0

LXQtTray::LXQtTray(QWidget *parent):
    QFrame(parent),
    mTrayId(0),
    mDamageEvent(0),
    mDamageError(0),
    mLayout(new QHBoxLayout(this)),
    mIconSize(style()->pixelMetric(QStyle::PM_ButtonIconSize)),
    mDisplay(QX11Info::display()),
    mScreen(QX11Info::appScreen()),
    mAtoms{
        XInternAtom(mDisplay, "MANAGER", false),
        XInternAtom(mDisplay, "_NET_SYSTEM_TRAY_ICON_SIZE", false),
        XInternAtom(mDisplay, "_NET_SYSTEM_TRAY_OPCODE", false),
        XInternAtom(mDisplay, "_NET_SYSTEM_TRAY_ORIENTATION", false),
        XInternAtom(mDisplay, QString("_NET_SYSTEM_TRAY_S%1").arg(mScreen).toUtf8(), false),
        XInternAtom(mDisplay, "_NET_SYSTEM_TRAY_VISUAL", false),
        XInternAtom(mDisplay, "_XEMBED", false),
        XInternAtom(mDisplay, "_XEMBED_INFO", false)
    }
{
    mLayout->setMargin(0);
    mLayout->setSpacing(3); /* TODO: scale by DPI */

    startTray();
}

LXQtTray::~LXQtTray()
{
    stopTray();
}

bool LXQtTray::nativeEventFilter(const QByteArray &eventType, void *message, long *)
{
    if (eventType != "xcb_generic_event_t")
        return false;

    xcb_generic_event_t* event = static_cast<xcb_generic_event_t *>(message);

    TrayIcon* icon;
    int event_type = event->response_type & ~0x80;

    switch (event_type)
    {
        case ClientMessage:
            clientMessageEvent(event);
            break;

        case DestroyNotify: {
            unsigned long event_window;
            event_window = reinterpret_cast<xcb_destroy_notify_event_t*>(event)->window;
            icon = findIcon(event_window);
            if (icon)
            {
                icon->windowDestroyed(event_window);
                mIcons.removeAll(icon);
                delete icon;
            }
            break;
        }
        default:
            if (event_type == mDamageEvent + XDamageNotify)
            {
                xcb_damage_notify_event_t* dmg = reinterpret_cast<xcb_damage_notify_event_t*>(event);
                icon = findIcon(dmg->drawable);
                if (icon)
                    icon->update();
            }
            break;
    }

    return false;
}

void LXQtTray::clientMessageEvent(xcb_generic_event_t *e)
{
    unsigned long opcode;
    unsigned long message_type;
    Window id;
    xcb_client_message_event_t* event = reinterpret_cast<xcb_client_message_event_t*>(e);
    uint32_t* data32 = event->data.data32;
    message_type = event->type;
    opcode = data32[1];
    if(message_type != mAtoms[_NET_SYSTEM_TRAY_OPCODE])
        return;

    switch (opcode)
    {
        case SYSTEM_TRAY_REQUEST_DOCK:
            id = data32[2];
            if (id)
                addIcon(id);
            break;
    }
}

TrayIcon* LXQtTray::findIcon(Window id)
{
    for(TrayIcon* icon : qAsConst(mIcons))
    {
        if (icon->iconId() == id || icon->windowId() == id)
            return icon;
    }
    return 0;
}

VisualID LXQtTray::getVisual()
{
    VisualID visualId = 0;
    Display* dsp = mDisplay;

    XVisualInfo templ;
    templ.screen = mScreen;
    templ.depth=32;
    templ.c_class=TrueColor;

    int nvi;
    XVisualInfo* xvi = XGetVisualInfo(dsp, VisualScreenMask|VisualDepthMask|VisualClassMask, &templ, &nvi);

    if (xvi)
    {
        int i;
        XRenderPictFormat* format;
        for (i = 0; i < nvi; i++)
        {
            format = XRenderFindVisualFormat(dsp, xvi[i].visual);
            if (format &&
                format->type == PictTypeDirect &&
                format->direct.alphaMask)
            {
                visualId = xvi[i].visualid;
                break;
            }
        }
        XFree(xvi);
    }

    return visualId;
}

void LXQtTray::startTray()
{
    Window root = QX11Info::appRootWindow();

    if (XGetSelectionOwner(mDisplay, mAtoms[_NET_SYSTEM_TRAY_Sn]) != None)
    {
        qWarning() << "Another systray is running";
        return;
    }

    // init systray protocol
    mTrayId = XCreateSimpleWindow(mDisplay, root, -1, -1, 1, 1, 0, 0, 0);

    XSetSelectionOwner(mDisplay, mAtoms[_NET_SYSTEM_TRAY_Sn], mTrayId, CurrentTime);
    if (XGetSelectionOwner(mDisplay, mAtoms[_NET_SYSTEM_TRAY_Sn]) != mTrayId)
    {
        qWarning() << "Can't get systray manager";
        stopTray();
        return;
    }

    int orientation = _NET_SYSTEM_TRAY_ORIENTATION_HORZ;
    XChangeProperty(mDisplay,
                    mTrayId,
                    mAtoms[_NET_SYSTEM_TRAY_ORIENTATION],
                    XA_CARDINAL,
                    32,
                    PropModeReplace,
                    (unsigned char *) &orientation,
                    1);

    VisualID visualId = getVisual();
    if (visualId)
    {
        XChangeProperty(mDisplay,
                        mTrayId,
                        mAtoms[_NET_SYSTEM_TRAY_VISUAL],
                        XA_VISUALID,
                        32,
                        PropModeReplace,
                        (unsigned char*)&visualId,
                        1);
    }

    XClientMessageEvent ev;
    ev.type = ClientMessage;
    ev.window = root;
    ev.message_type = mAtoms[MANAGER];
    ev.format = 32;
    ev.data.l[0] = CurrentTime;
    ev.data.l[1] = mAtoms[_NET_SYSTEM_TRAY_Sn];
    ev.data.l[2] = mTrayId;
    ev.data.l[3] = 0;
    ev.data.l[4] = 0;
    XSendEvent(mDisplay, root, False, StructureNotifyMask, (XEvent*)&ev);

    XDamageQueryExtension(mDisplay, &mDamageEvent, &mDamageError);

    qApp->installNativeEventFilter(this);
}

void LXQtTray::stopTray()
{
    for (auto & icon : mIcons)
        disconnect(icon, &QObject::destroyed, this, &LXQtTray::onIconDestroyed);
    qDeleteAll(mIcons);
    if (mTrayId)
    {
        XDestroyWindow(mDisplay, mTrayId);
        mTrayId = 0;
    }
}

void LXQtTray::onIconDestroyed(QObject * icon)
{
    //in the time QOjbect::destroyed is emitted, the child destructor
    //is already finished, so the qobject_cast to child will return nullptr in all cases
    mIcons.removeAll(static_cast<TrayIcon *>(icon));
}

void LXQtTray::addIcon(Window winId)
{
    // decline to add an icon for a window we already manage
    TrayIcon *icon = findIcon(winId);
    if(icon)
        return;

    icon = new TrayIcon(winId, this);
    mIcons.append(icon);
    connect(icon, &QObject::destroyed, this, &LXQtTray::onIconDestroyed);

    // add in sorted order
    int idx = 0;
    for(; idx < mLayout->count(); idx++)
    {
        auto icon2 = static_cast<TrayIcon *>(mLayout->itemAt(idx)->widget());
        if (icon->appName() < icon2->appName())
            break;
    }
    mLayout->insertWidget(idx, icon);
}
