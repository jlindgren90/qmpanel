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
#include <QBitmap>
#include <QDebug>
#include <QPainter>
#include <QResizeEvent>
#include <QScreen>
#include <QStyle>
#include <QTimer>

#include "systray.h"
#include "trayicon.h"

#include <KWindowInfo>
#include <QX11Info>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xcomposite.h>

#define XEMBED_EMBEDDED_NOTIFY 0

static bool xError;

int windowErrorHandler(Display * d, XErrorEvent * e)
{
    xError = true;
    if (e->error_code != BadWindow)
    {
        char str[1024];
        XGetErrorText(d, e->error_code, str, 1024);
        qWarning() << "Error handler" << e->error_code << str;
    }
    return 0;
}

TrayIcon::TrayIcon(Window iconId, SysTray * tray)
    : QWidget(tray), mTray(tray), mIconSize(tray->iconSize()), mIconId(iconId),
      mAppName(KWindowInfo(iconId, 0, NET::WM2WindowClass).windowClassName()),
      mDisplay(QX11Info::display())
{
    setFixedSize(mIconSize, mIconSize);

    QTimer::singleShot(200, [this] {
        init();
        update();
    });
}

void TrayIcon::init()
{
    Display * dsp = mDisplay;

    XWindowAttributes attr;
    if (!XGetWindowAttributes(dsp, mIconId, &attr))
    {
        deleteLater();
        return;
    }

    unsigned long mask = 0;
    XSetWindowAttributes set_attr;

    Visual * visual = attr.visual;
    set_attr.colormap = attr.colormap;
    set_attr.background_pixel = 0;
    set_attr.border_pixel = 0;
    mask = CWColormap | CWBackPixel | CWBorderPixel;

    int sizeDevPx = mIconSize * devicePixelRatioF();
    mWindowId = XCreateWindow(dsp, this->winId(), 0, 0, sizeDevPx, sizeDevPx, 0,
                              attr.depth, InputOutput, visual, mask, &set_attr);

    xError = false;
    XErrorHandler old;
    old = XSetErrorHandler(windowErrorHandler);
    XReparentWindow(dsp, mIconId, mWindowId, 0, 0);
    XSync(dsp, false);
    XSetErrorHandler(old);

    if (xError)
    {
        qWarning() << "Can't reparent icon window";
        return;
    }

    {
        XEvent e;
        e.xclient.type = ClientMessage;
        e.xclient.serial = 0;
        e.xclient.send_event = True;
        e.xclient.message_type = mTray->atom(SysTray::_XEMBED);
        e.xclient.window = mIconId;
        e.xclient.format = 32;
        e.xclient.data.l[0] = CurrentTime;
        e.xclient.data.l[1] = XEMBED_EMBEDDED_NOTIFY;
        e.xclient.data.l[2] = 0;
        e.xclient.data.l[3] = mWindowId;
        e.xclient.data.l[4] = 0;
        XSendEvent(dsp, mIconId, false, 0xFFFFFF, &e);
    }

    XSelectInput(dsp, mIconId, StructureNotifyMask);
    mDamage = XDamageCreate(dsp, mIconId, XDamageReportRawRectangles);
    XCompositeRedirectWindow(dsp, mWindowId, CompositeRedirectManual);

    XMapWindow(dsp, mIconId);
    XMapRaised(dsp, mWindowId);
    XResizeWindow(dsp, mIconId, sizeDevPx, sizeDevPx);
}

TrayIcon::~TrayIcon()
{
    Display * dsp = mDisplay;
    XSelectInput(dsp, mIconId, NoEventMask);

    if (mDamage)
        XDamageDestroy(dsp, mDamage);

    // reparent to root
    xError = false;
    XErrorHandler old = XSetErrorHandler(windowErrorHandler);

    XUnmapWindow(dsp, mIconId);
    XReparentWindow(dsp, mIconId, QX11Info::appRootWindow(), 0, 0);

    if (mWindowId)
        XDestroyWindow(dsp, mWindowId);
    XSync(dsp, False);
    XSetErrorHandler(old);
}

void TrayIcon::paintEvent(QPaintEvent *)
{
    if (!mWindowId)
        return;

    XWindowAttributes attr, attr2;
    if (!XGetWindowAttributes(mDisplay, mIconId, &attr) ||
        !XGetWindowAttributes(mDisplay, mWindowId, &attr2))
    {
        return;
    }

    XImage * ximage =
        XGetImage(mDisplay, mIconId, 0, 0, qMin(attr.width, attr2.width),
                  qMin(attr.height, attr2.height), AllPlanes, ZPixmap);
    if (!ximage)
        return;

    QImage image((const uchar *)ximage->data, ximage->width, ximage->height,
                 ximage->bytes_per_line, QImage::Format_ARGB32_Premultiplied);
    image.setDevicePixelRatio(devicePixelRatioF());

    QRect drawRect(QPoint(), image.size() / devicePixelRatioF());
    drawRect.moveCenter(rect().center());

    QPainter painter(this);
    painter.drawImage(drawRect.topLeft(), image);

    XDestroyImage(ximage);
}

void TrayIcon::windowDestroyed(Window w)
{
    // damage is destroyed if its parent window was destroyed
    if (mIconId == w)
        mDamage = 0;
}
