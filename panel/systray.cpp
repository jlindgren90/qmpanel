/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * qmpanel - a minimal Qt-based desktop panel
 *
 * Copyright: 2011 Razor team
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

#include "systray.h"
#include "trayicon.h"

#include <QApplication>
#include <QDebug>
#include <QX11Info>
#include <memory>

#include <X11/Xatom.h>
#include <X11/extensions/Xdamage.h>
#include <X11/extensions/Xrender.h>
#include <xcb/damage.h>
#include <xcb/xcb.h>

#define _NET_SYSTEM_TRAY_ORIENTATION_HORZ 0
#define SYSTEM_TRAY_REQUEST_DOCK 0

SysTray::SysTray(QWidget * parent)
    : QWidget(parent), mLayout(this), mDisplay(QX11Info::display()),
      mScreen(QX11Info::appScreen()),
      mAtoms{XInternAtom(mDisplay, "MANAGER", false),
             XInternAtom(mDisplay, "_NET_SYSTEM_TRAY_ICON_SIZE", false),
             XInternAtom(mDisplay, "_NET_SYSTEM_TRAY_OPCODE", false),
             XInternAtom(mDisplay, "_NET_SYSTEM_TRAY_ORIENTATION", false),
             XInternAtom(mDisplay,
                         QString("_NET_SYSTEM_TRAY_S%1").arg(mScreen).toUtf8(),
                         false),
             XInternAtom(mDisplay, "_NET_SYSTEM_TRAY_VISUAL", false)}
{
    mLayout.setContentsMargins(QMargins());
    mLayout.setSpacing(logicalDpiX() / 24);

    Window root = QX11Info::appRootWindow();
    VisualID visualId = getVisual();

    if (!visualId)
    {
        qWarning() << "Can't find usable visual";
        return;
    }

    if (!XDamageQueryExtension(mDisplay, &mDamageEvent, &mDamageError))
    {
        qWarning() << "Damage extension not present";
        return;
    }

    if (XGetSelectionOwner(mDisplay, mAtoms[_NET_SYSTEM_TRAY_Sn]) != None)
    {
        qWarning() << "Another systray is running";
        return;
    }

    qApp->installNativeEventFilter(this);

    mTrayId = XCreateSimpleWindow(mDisplay, root, -1, -1, 1, 1, 0, 0, 0);
    XSetSelectionOwner(mDisplay, mAtoms[_NET_SYSTEM_TRAY_Sn], mTrayId,
                       CurrentTime);

    if (XGetSelectionOwner(mDisplay, mAtoms[_NET_SYSTEM_TRAY_Sn]) != mTrayId)
    {
        qWarning() << "Can't get systray selection";
        return;
    }

    int orientation = _NET_SYSTEM_TRAY_ORIENTATION_HORZ;
    XChangeProperty(mDisplay, mTrayId, mAtoms[_NET_SYSTEM_TRAY_ORIENTATION],
                    XA_CARDINAL, 32, PropModeReplace,
                    (unsigned char *)&orientation, 1);
    XChangeProperty(mDisplay, mTrayId, mAtoms[_NET_SYSTEM_TRAY_VISUAL],
                    XA_VISUALID, 32, PropModeReplace,
                    (unsigned char *)&visualId, 1);

    XClientMessageEvent ev{};
    ev.type = ClientMessage;
    ev.window = root;
    ev.message_type = mAtoms[MANAGER];
    ev.format = 32;
    ev.data.l[0] = CurrentTime;
    ev.data.l[1] = mAtoms[_NET_SYSTEM_TRAY_Sn];
    ev.data.l[2] = mTrayId;

    XSendEvent(mDisplay, root, False, StructureNotifyMask, (XEvent *)&ev);
}

SysTray::~SysTray()
{
    while (!mLayout.isEmpty())
        delete mLayout.itemAt(0)->widget();

    if (mTrayId)
        XDestroyWindow(mDisplay, mTrayId);
}

bool SysTray::nativeEventFilter(const QByteArray & eventType, void * message,
                                long *)
{
    if (eventType != "xcb_generic_event_t")
        return false;

    auto event = (xcb_generic_event_t *)message;
    int event_type = event->response_type & ~0x80;

    if (event_type == ClientMessage)
    {
        auto cmev = (xcb_client_message_event_t *)event;
        if (cmev->type == mAtoms[_NET_SYSTEM_TRAY_OPCODE] &&
            cmev->data.data32[1] == SYSTEM_TRAY_REQUEST_DOCK &&
            cmev->data.data32[2])
        {
            addIcon(cmev->data.data32[2]);
        }
    }
    else if (event_type == DestroyNotify)
    {
        auto dnev = (xcb_destroy_notify_event_t *)event;
        auto icon = findIcon(dnev->window);
        if (icon)
        {
            icon->windowDestroyed(dnev->window);
            delete icon;
        }
    }
    else if (event_type == mDamageEvent + XDamageNotify)
    {
        auto dnev = (xcb_damage_notify_event_t *)event;
        auto icon = findIcon(dnev->drawable);
        if (icon)
            icon->update();
    }

    return false;
}

TrayIcon * SysTray::findIcon(Window id)
{
    for (int idx = 0; idx < mLayout.count(); idx++)
    {
        auto item = mLayout.itemAt(idx);
        auto icon = static_cast<TrayIcon *>(item->widget());
        if (icon->iconId() == id || icon->windowId() == id)
            return icon;
    }

    return nullptr;
}

VisualID SysTray::getVisual()
{
    XVisualInfo templ{};
    templ.screen = mScreen;
    templ.depth = 32;
    templ.c_class = TrueColor;

    int nvi = 0;
    auto mask = VisualScreenMask | VisualDepthMask | VisualClassMask;
    std::unique_ptr<XVisualInfo[], decltype(&XFree)> xvi(
        XGetVisualInfo(mDisplay, mask, &templ, &nvi), XFree);

    for (int i = 0; xvi && i < nvi; i++)
    {
        auto format = XRenderFindVisualFormat(mDisplay, xvi[i].visual);
        if (format && format->type == PictTypeDirect &&
            format->direct.alphaMask)
        {
            return xvi[i].visualid;
        }
    }

    return 0;
}

void SysTray::addIcon(Window winId)
{
    if (findIcon(winId))
        return;

    auto icon = new TrayIcon(winId, this);

    // add in sorted order
    int idx = 0;
    for (; idx < mLayout.count(); idx++)
    {
        auto icon2 = static_cast<TrayIcon *>(mLayout.itemAt(idx)->widget());
        if (icon->appName().compare(icon2->appName(), Qt::CaseInsensitive) < 0)
            break;
    }

    mLayout.insertWidget(idx, icon);
}
