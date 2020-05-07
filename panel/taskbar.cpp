/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * qmpanel - a minimal Qt-based desktop panel
 *
 * Copyright: 2011 Razor team
 *            2014 LXQt team
 *            2020 John Lindgren
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

#include <KWindowSystem>
#include <QX11Info>

TaskBar::TaskBar(QWidget * parent) : QWidget(parent), mLayout(this)
{
    mLayout.setMargin(0);
    mLayout.setSpacing(0);
    mLayout.addStretch(1);

    setAcceptDrops(true);

    for (auto window : KWindowSystem::stackingOrder())
    {
        if (acceptWindow(window))
            addWindow(window);
    }

    connect(KWindowSystem::self(), &KWindowSystem::windowAdded, this,
            &TaskBar::onWindowAdded);
    connect(KWindowSystem::self(), &KWindowSystem::windowRemoved, this,
            &TaskBar::removeWindow);
    connect(KWindowSystem::self(), &KWindowSystem::activeWindowChanged, this,
            &TaskBar::onActiveWindowChanged);

    void (KWindowSystem::*windowChanged)(
        WId, NET::Properties, NET::Properties2) = &KWindowSystem::windowChanged;
    connect(KWindowSystem::self(), windowChanged, this,
            &TaskBar::onWindowChanged);
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
        auto button = new TaskButton(window, this);
        mLayout.insertWidget(mLayout.count() - 1, button);
        mKnownWindows[window] = button;
    }
}

void TaskBar::removeWindow(WId window)
{
    auto pos = mKnownWindows.find(window);
    if (pos != mKnownWindows.end())
    {
        auto button = *pos;
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
    auto active = mKnownWindows.value(window);
    if (!active)
    {
        KWindowInfo info(window, 0, NET::WM2TransientFor);
        active = mKnownWindows.value(info.transientFor());
    }

    for (auto button : mKnownWindows)
    {
        if (button != active)
            button->setChecked(false);
    }

    if (active)
        active->setChecked(true);
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

    auto button = mKnownWindows.value(window);
    if (!button)
        return;

    if (prop.testFlag(NET::WMVisibleName) || prop.testFlag(NET::WMName))
        button->updateText();
    if (prop.testFlag(NET::WMIcon))
        button->updateIcon();
}
