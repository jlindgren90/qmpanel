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

#include "taskbar.h"
#include "taskbutton.h"
#include "wlr-foreign-toplevel-management-unstable-v1.h"

#include <KWindowInfo>
#include <KX11Extras>
#include <QGuiApplication>
#include <private/qtx11extras_p.h>

TaskBar::TaskBar(Resources & res, QWidget * parent)
    : QWidget(parent), mRes(res), mLayout(this)
{
    mLayout.setContentsMargins(QMargins());
    mLayout.setSpacing(0);
    mLayout.addStretch(1);

    setAcceptDrops(true);

    if (QX11Info::isPlatformX11())
    {
        for (auto window : KX11Extras::stackingOrder())
        {
            if (acceptWindow(window))
                addWindow(window);
        }

        connect(KX11Extras::self(), &KX11Extras::windowAdded, this,
                &TaskBar::onWindowAdded);
        connect(KX11Extras::self(), &KX11Extras::windowRemoved, this,
                &TaskBar::removeWindow);
        connect(KX11Extras::self(), &KX11Extras::activeWindowChanged, this,
                &TaskBar::onActiveWindowChanged);
        connect(KX11Extras::self(), &KX11Extras::windowChanged, this,
                &TaskBar::onWindowChanged);
    }

    auto waylandApp =
        qGuiApp->nativeInterface<QNativeInterface::QWaylandApplication>();
    if (waylandApp)
    {
        static const wl_registry_listener registry_listener_impl = {
            .global =
                [](void * data, wl_registry * registry, uint32_t name,
                   const char * interface, uint32_t version) {
                    auto self = static_cast<TaskBar *>(data);
                    if (!strcmp(
                            interface,
                            zwlr_foreign_toplevel_manager_v1_interface.name))
                        self->addToplevelManager(registry, name, version);
                },
            .global_remove = [](void * data, wl_registry * registry,
                                uint32_t name) { /* no-op */ }};

        wl_registry_add_listener(wl_display_get_registry(waylandApp->display()),
                                 &registry_listener_impl, this);
        // TODO: cleanup listener at exit?
    }
}

void TaskBar::addToplevelManager(wl_registry * registry, uint32_t name,
                                 uint32_t version)
{
    version = std::min<uint32_t>(
        version, zwlr_foreign_toplevel_manager_v1_interface.version);
    auto manager = static_cast<zwlr_foreign_toplevel_manager_v1 *>(
        wl_registry_bind(registry, name,
                         &zwlr_foreign_toplevel_manager_v1_interface, version));
    if (!manager)
    {
        qWarning()
            << "Could not bind zwlr_foreign_toplevel_manager_v1_interface";
        return;
    }

    static const zwlr_foreign_toplevel_manager_v1_listener
        toplevel_manager_impl = {
            .toplevel =
                [](void * data, zwlr_foreign_toplevel_manager_v1 * manager,
                   zwlr_foreign_toplevel_handle_v1 * handle) {
                    static_cast<TaskBar *>(data)->addWindow(handle);
                },
            .finished =
                [](void * data, zwlr_foreign_toplevel_manager_v1 * manager) {
                    /* no-op */
                },
        };

    zwlr_foreign_toplevel_manager_v1_add_listener(manager,
                                                  &toplevel_manager_impl, this);
    // TODO: cleanup listener at exit?
}

void TaskBar::addWindow(zwlr_foreign_toplevel_handle_v1 * handle)
{
    auto button = new TaskButtonWayland(mRes, handle, this);
    mLayout.insertWidget(mLayout.count() - 1, button);
}

bool TaskBar::acceptWindow(WId window) const
{
    const NET::WindowTypes ignoreList =
        NET::DesktopMask | NET::DockMask | NET::SplashMask | NET::ToolbarMask |
        NET::MenuMask | NET::PopupMenuMask | NET::NotificationMask;

    KWindowInfo info(window, NET::WMWindowType | NET::WMState,
                     NET::WM2TransientFor);

    if (!info.valid() ||
        NET::typeMatchesMask(info.windowType(NET::AllTypesMask), ignoreList) ||
        (info.state() & NET::SkipTaskbar))
    {
        return false;
    }

    WId transFor = info.transientFor();
    if (transFor == 0 || transFor == window ||
        transFor == (WId)QX11Info::appRootWindow())
    {
        return true;
    }

    return false;
}

void TaskBar::addWindow(WId window)
{
    if (mKnownWindows.find(window) == mKnownWindows.end())
    {
        auto button = new TaskButtonX11(window, this);
        mLayout.insertWidget(mLayout.count() - 1, button);
        mKnownWindows[window] = button;
    }
}

void TaskBar::removeWindow(WId window)
{
    auto pos = mKnownWindows.find(window);
    if (pos != mKnownWindows.end())
    {
        auto button = pos->second;
        mKnownWindows.erase(pos);
        delete button;
    }
}

void TaskBar::onWindowAdded(WId window)
{
    auto pos = mKnownWindows.find(window);
    if (pos == mKnownWindows.end() && acceptWindow(window))
        addWindow(window);
}

void TaskBar::onActiveWindowChanged(WId window)
{
    auto active = mKnownWindows.find(window);
    if (active == mKnownWindows.end())
    {
        KWindowInfo info(window, NET::Properties(), NET::WM2TransientFor);
        active = mKnownWindows.find(info.transientFor());
    }

    for (auto & pair : mKnownWindows)
    {
        if (active == mKnownWindows.end() || pair.second != active->second)
            pair.second->setChecked(false);
    }

    if (active != mKnownWindows.end())
        active->second->setChecked(true);
}

void TaskBar::onWindowChanged(WId window, NET::Properties prop,
                              NET::Properties2 prop2)
{
    if (prop.testFlag(NET::WMWindowType) || prop.testFlag(NET::WMState) ||
        prop2.testFlag(NET::WM2TransientFor))
    {
        if (acceptWindow(window))
            addWindow(window);
        else
            removeWindow(window);
    }

    auto pos = mKnownWindows.find(window);
    if (pos == mKnownWindows.end())
        return;

    if (prop.testFlag(NET::WMVisibleName) || prop.testFlag(NET::WMName))
        pos->second->updateText();
    if (prop.testFlag(NET::WMIcon))
        pos->second->updateIcon();
}
