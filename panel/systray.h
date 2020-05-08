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

#ifndef SYSTRAY_H
#define SYSTRAY_H

#include <QAbstractNativeEventFilter>
#include <QHBoxLayout>
#include <QWidget>

#include <X11/X.h>
#include <X11/Xlib.h>

#include <fixx11h.h>

class TrayIcon;
class QHBoxLayout;

class SysTray : public QWidget, QAbstractNativeEventFilter
{
public:
    explicit SysTray(QWidget * parent);
    ~SysTray();

    bool nativeEventFilter(const QByteArray & eventType, void * message,
                           long *) override;

private:
    enum
    {
        MANAGER,
        _NET_SYSTEM_TRAY_ICON_SIZE,
        _NET_SYSTEM_TRAY_OPCODE,
        _NET_SYSTEM_TRAY_ORIENTATION,
        _NET_SYSTEM_TRAY_Sn,
        _NET_SYSTEM_TRAY_VISUAL,
        NUM_ATOMS
    };

    VisualID getVisual();
    void addIcon(Window id);
    TrayIcon * findIcon(Window trayId);

    Window mTrayId = 0;
    int mDamageEvent = 0;
    int mDamageError = 0;
    QHBoxLayout mLayout;
    Display * const mDisplay;
    int const mScreen;
    Atom const mAtoms[NUM_ATOMS];
};

#endif
