/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2010-2011 Razor team
 * Authors:
 *   Christopher "VdoP" Regali
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

#include <QHash>
#include <QX11Info>

#include "xfitman.h"
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

/**
 * @file xfitman.cpp
 * @brief implements class Xfitman
 * @author Christopher "VdoP" Regali
 */

Atom XfitMan::atom(const char* atomName)
{
    static QHash<QString, Atom> hash;
    if (hash.contains(atomName))
        return hash.value(atomName);
    Atom atom = XInternAtom(QX11Info::display(), atomName, false);
    hash[atomName] = atom;
    return atom;
}

/**
 * @brief moves a window to a new position
 */

void XfitMan::moveWindow(Window _win, int _x, int _y)
{
    XMoveWindow(QX11Info::display(), _win, _x, _y);
}

/**
 * @brief resizes a window to the given dimensions
 */
void XfitMan::resizeWindow(Window _wid, int _width, int _height)
{
    XResizeWindow(QX11Info::display(), _wid, _width, _height);
}

QString XfitMan::getApplicationName(Window _wid)
{
    XClassHint hint;
    QString ret;

    if (XGetClassHint(QX11Info::display(), _wid, &hint))
    {
        if (hint.res_name)
        {
            ret = hint.res_name;
            XFree(hint.res_name);
        }
        if (hint.res_class)
        {
            XFree(hint.res_class);
        }
    }

    return ret;
}
