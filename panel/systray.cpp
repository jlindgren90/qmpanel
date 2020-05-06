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
#include <QX11Info>

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xdamage.h>
#include <X11/extensions/Xrender.h>
#include <xcb/damage.h>
#include <xcb/xcb.h>

#include "systray.h"
#include "trayicon.h"

#define _NET_SYSTEM_TRAY_ORIENTATION_HORZ 0
#define SYSTEM_TRAY_REQUEST_DOCK 0

SysTray::SysTray(QWidget * parent)
    : QFrame(parent), mLayout(new QHBoxLayout(this)),
      mIconSize(style()->pixelMetric(QStyle::PM_ButtonIconSize)),
      mDisplay(QX11Info::display()), mScreen(QX11Info::appScreen()),
      mAtoms{XInternAtom(mDisplay, "MANAGER", false),
             XInternAtom(mDisplay, "_NET_SYSTEM_TRAY_ICON_SIZE", false),
             XInternAtom(mDisplay, "_NET_SYSTEM_TRAY_OPCODE", false),
             XInternAtom(mDisplay, "_NET_SYSTEM_TRAY_ORIENTATION", false),
             XInternAtom(mDisplay,
                         QString("_NET_SYSTEM_TRAY_S%1").arg(mScreen).toUtf8(),
                         false),
             XInternAtom(mDisplay, "_NET_SYSTEM_TRAY_VISUAL", false),
             XInternAtom(mDisplay, "_XEMBED", false),
             XInternAtom(mDisplay, "_XEMBED_INFO", false)}
{
    mLayout->setMargin(0);
    mLayout->setSpacing(3); /* TODO: scale by DPI */

    Window root = QX11Info::appRootWindow();

    if (XGetSelectionOwner(mDisplay, mAtoms[_NET_SYSTEM_TRAY_Sn]) != None)
    {
        qWarning() << "Another systray is running";
        return;
    }

    mTrayId = XCreateSimpleWindow(mDisplay, root, -1, -1, 1, 1, 0, 0, 0);
    XSetSelectionOwner(mDisplay, mAtoms[_NET_SYSTEM_TRAY_Sn], mTrayId,
                       CurrentTime);
    if (XGetSelectionOwner(mDisplay, mAtoms[_NET_SYSTEM_TRAY_Sn]) != mTrayId)
    {
        qWarning() << "Can't get systray manager";
        return;
    }

    int orientation = _NET_SYSTEM_TRAY_ORIENTATION_HORZ;
    XChangeProperty(mDisplay, mTrayId, mAtoms[_NET_SYSTEM_TRAY_ORIENTATION],
                    XA_CARDINAL, 32, PropModeReplace,
                    (unsigned char *)&orientation, 1);

    VisualID visualId = getVisual();
    if (visualId)
        XChangeProperty(mDisplay, mTrayId, mAtoms[_NET_SYSTEM_TRAY_VISUAL],
                        XA_VISUALID, 32, PropModeReplace,
                        (unsigned char *)&visualId, 1);

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
    XSendEvent(mDisplay, root, False, StructureNotifyMask, (XEvent *)&ev);

    XDamageQueryExtension(mDisplay, &mDamageEvent, &mDamageError);

    qApp->installNativeEventFilter(this);
}

SysTray::~SysTray()
{
    while (!mLayout->isEmpty())
        delete mLayout->itemAt(0)->widget();

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
        clientMessageEvent(event);
    }
    else if (event_type == DestroyNotify)
    {
        auto event_window = ((xcb_destroy_notify_event_t *)event)->window;
        auto icon = findIcon(event_window);
        if (icon)
        {
            icon->windowDestroyed(event_window);
            delete icon;
        }
    }
    else if (event_type == mDamageEvent + XDamageNotify)
    {
        auto drawable = ((xcb_damage_notify_event_t *)event)->drawable;
        auto icon = findIcon(drawable);
        if (icon)
            icon->update();
    }

    return false;
}

void SysTray::clientMessageEvent(xcb_generic_event_t * e)
{
    auto event = (xcb_client_message_event_t *)e;

    if (event->type == mAtoms[_NET_SYSTEM_TRAY_OPCODE] &&
        event->data.data32[1] == SYSTEM_TRAY_REQUEST_DOCK &&
        event->data.data32[2])
    {
        addIcon(event->data.data32[2]);
    }
}

TrayIcon * SysTray::findIcon(Window id)
{
    for (int idx = 0; idx < mLayout->count(); idx++)
    {
        auto item = mLayout->itemAt(idx);
        auto icon = static_cast<TrayIcon *>(item->widget());
        if (icon->iconId() == id || icon->windowId() == id)
            return icon;
    }

    return nullptr;
}

VisualID SysTray::getVisual()
{
    XVisualInfo templ;
    templ.screen = mScreen;
    templ.depth = 32;
    templ.c_class = TrueColor;

    int nvi;
    auto mask = VisualScreenMask | VisualDepthMask | VisualClassMask;
    std::unique_ptr<XVisualInfo[], decltype(&XFree)> xvi(
        XGetVisualInfo(mDisplay, mask, &templ, &nvi), XFree);

    if (xvi)
    {
        for (int i = 0; i < nvi; i++)
        {
            auto format = XRenderFindVisualFormat(mDisplay, xvi[i].visual);
            if (format && format->type == PictTypeDirect &&
                format->direct.alphaMask)
            {
                return xvi[i].visualid;
            }
        }
    }

    return 0;
}

void SysTray::addIcon(Window winId)
{
    // decline to add an icon for a window we already manage
    TrayIcon * icon = findIcon(winId);
    if (icon)
        return;

    icon = new TrayIcon(winId, this);

    // add in sorted order
    int idx = 0;
    for (; idx < mLayout->count(); idx++)
    {
        auto icon2 = static_cast<TrayIcon *>(mLayout->itemAt(idx)->widget());
        if (icon->appName() < icon2->appName())
            break;
    }
    mLayout->insertWidget(idx, icon);
}
