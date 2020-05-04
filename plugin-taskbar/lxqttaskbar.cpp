/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2011 Razor team
 *            2014 LXQt team
 * Authors:
 *   Alexander Sokoloff <sokoloff.a@gmail.com>
 *   Maciej Płaza <plaza.maciej@gmail.com>
 *   Kuzma Shapran <kuzma.shapran@gmail.com>
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

#include "lxqttaskbar.h"

#include <KWindowSystem>
#include <QX11Info>

#include "../panel/lxqtpanel.h"

LXQtTaskBar::LXQtTaskBar(LXQtPanel * panel)
    : QWidget(panel), mPanel(panel), mLayout(this)
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
            &LXQtTaskBar::onWindowAdded);
    connect(KWindowSystem::self(), &KWindowSystem::windowRemoved, this,
            &LXQtTaskBar::removeWindow);
    connect(KWindowSystem::self(), &KWindowSystem::activeWindowChanged, this,
            &LXQtTaskBar::onActiveWindowChanged);

    void (KWindowSystem::*windowChanged)(
        WId, NET::Properties, NET::Properties2) = &KWindowSystem::windowChanged;
    connect(KWindowSystem::self(), windowChanged, this,
            &LXQtTaskBar::onWindowChanged);
}

bool LXQtTaskBar::acceptWindow(WId window) const
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

void LXQtTaskBar::addWindow(WId window)
{
    if (mKnownWindows.find(window) == mKnownWindows.end())
    {
        auto button = new LXQtTaskButton(window, mPanel, this);
        mLayout.insertWidget(mLayout.count() - 1, button);
        mKnownWindows[window] = button;
    }
}

void LXQtTaskBar::removeWindow(WId window)
{
    auto pos = mKnownWindows.find(window);
    if (pos != mKnownWindows.end())
    {
        auto button = *pos;
        mKnownWindows.erase(pos);
        delete button;
    }
}

void LXQtTaskBar::onWindowAdded(WId window)
{
    auto pos = mKnownWindows.find(window);
    if (pos == mKnownWindows.end() && acceptWindow(window))
        addWindow(window);
}

void LXQtTaskBar::onActiveWindowChanged(WId window)
{
    auto active = mKnownWindows.value(window);
    if(!active)
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

void LXQtTaskBar::onWindowChanged(WId window, NET::Properties prop,
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
