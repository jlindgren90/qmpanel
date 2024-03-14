/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * qmpanel - a minimal Qt-based desktop panel
 *
 * Copyright: 2011 Razor team
 *            2014 LXQt team
 *            2020-2024 John Lindgren
 * Authors:
 *   Alexander Sokoloff <sokoloff.a@gmail.com>
 *   Maciej Płaza <plaza.maciej@gmail.com>
 *   Kuzma Shapran <kuzma.shapran@gmail.com>
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

#ifndef TASKBAR_H
#define TASKBAR_H

#include <NETWM>
#include <QHBoxLayout>
#include <QWidget>
#include <unordered_map>

class Resources;
class TaskButtonX11;
class TaskButtonWayland;

struct wl_registry;
struct zwlr_foreign_toplevel_handle_v1;
struct zwlr_foreign_toplevel_manager_v1;

class TaskBar : public QWidget
{
public:
    explicit TaskBar(Resources & res, QWidget * parent);

    // Wayland-specific
    void addToplevelManager(wl_registry * registry, uint32_t name,
                            uint32_t version);
    void addWindow(zwlr_foreign_toplevel_handle_v1 * handle);

private:
    // X11-specific
    bool acceptWindow(WId window) const;
    void addWindow(WId window);
    void removeWindow(WId window);
    void onWindowAdded(WId window);
    void onActiveWindowChanged(WId window);
    void onWindowChanged(WId window, NET::Properties prop,
                         NET::Properties2 prop2);

    Resources & mRes;
    std::unordered_map<WId, TaskButtonX11 *> mKnownWindows;
    QHBoxLayout mLayout;
};

#endif // TASKBAR_H
