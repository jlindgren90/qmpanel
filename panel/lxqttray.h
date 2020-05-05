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

#ifndef LXQTTRAY_H
#define LXQTTRAY_H

#include <QFrame>
#include <QAbstractNativeEventFilter>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <xcb/xcb_event.h>
#include "fixx11h.h"

class TrayIcon;
class QHBoxLayout;

class LXQtTray: public QFrame, QAbstractNativeEventFilter
{
public:
    enum {
        MANAGER,
        _NET_SYSTEM_TRAY_ICON_SIZE,
        _NET_SYSTEM_TRAY_OPCODE,
        _NET_SYSTEM_TRAY_ORIENTATION,
        _NET_SYSTEM_TRAY_Sn,
        _NET_SYSTEM_TRAY_VISUAL,
        _XEMBED,
        _XEMBED_INFO,
        NUM_ATOMS
    };

    LXQtTray(QWidget* parent);
    ~LXQtTray();

    Atom atom(int idx) const { return mAtoms[idx]; }
    int iconSize() const { return mIconSize; }

    bool nativeEventFilter(const QByteArray &eventType, void *message, long *);

private:
    void startTray();
    void stopTray();
    void onIconDestroyed(QObject * icon);

    VisualID getVisual();

    void clientMessageEvent(xcb_generic_event_t *e);

    int clientMessage(WId _wid, Atom _msg,
                      long unsigned int data0,
                      long unsigned int data1 = 0,
                      long unsigned int data2 = 0,
                      long unsigned int data3 = 0,
                      long unsigned int data4 = 0) const;

    void addIcon(Window id);

    TrayIcon* findIcon(Window trayId);

    Window mTrayId;
    QList<TrayIcon*> mIcons;
    int mDamageEvent;
    int mDamageError;
    QHBoxLayout * const mLayout;
    int const mIconSize;
    Display * const mDisplay;
    int const mScreen;
    Atom const mAtoms[NUM_ATOMS];
};


#endif
