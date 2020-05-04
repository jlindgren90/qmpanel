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

#include <QX11Info>
#include <KWindowSystem>

#include "lxqttaskbar.h"

/************************************************

************************************************/
LXQtTaskBar::LXQtTaskBar(Plugin *plugin, QWidget *parent) :
    QWidget(parent),
    mLayout(this),
    mPlugin(plugin)
{
    mLayout.setMargin(0);
    mLayout.setSpacing(0);

    setAcceptDrops(true);

    for (auto window: KWindowSystem::stackingOrder())
    {
        if (acceptWindow(window))
            addWindow(window);
    }

    connect(KWindowSystem::self(), &KWindowSystem::windowAdded, this, &LXQtTaskBar::onWindowAdded);
    connect(KWindowSystem::self(), &KWindowSystem::windowRemoved, this, &LXQtTaskBar::onWindowRemoved);
    connect(KWindowSystem::self(), &KWindowSystem::activeWindowChanged, this, &LXQtTaskBar::onActiveWindowChanged);
    connect(KWindowSystem::self(), static_cast<void (KWindowSystem::*)(WId, NET::Properties, NET::Properties2)>(&KWindowSystem::windowChanged)
            , this, &LXQtTaskBar::onWindowChanged);
}

/************************************************

 ************************************************/
bool LXQtTaskBar::acceptWindow(WId window) const
{
    QFlags<NET::WindowTypeMask> ignoreList;
    ignoreList |= NET::DesktopMask;
    ignoreList |= NET::DockMask;
    ignoreList |= NET::SplashMask;
    ignoreList |= NET::ToolbarMask;
    ignoreList |= NET::MenuMask;
    ignoreList |= NET::PopupMenuMask;
    ignoreList |= NET::NotificationMask;

    KWindowInfo info(window, NET::WMWindowType | NET::WMState, NET::WM2TransientFor);
    if (!info.valid())
        return false;

    if (NET::typeMatchesMask(info.windowType(NET::AllTypesMask), ignoreList))
        return false;

    if (info.state() & NET::SkipTaskbar)
        return false;

    // WM_TRANSIENT_FOR hint not set - normal window
    WId transFor = info.transientFor();
    if (transFor == 0 || transFor == window || transFor == (WId) QX11Info::appRootWindow())
        return true;

    return false;
}

/************************************************

 ************************************************/
void LXQtTaskBar::addWindow(WId window)
{
    if (mKnownWindows.find(window) == mKnownWindows.end())
    {
        auto group = new LXQtTaskButton(window, this, this);
        mKnownWindows[window] = group;
        mLayout.addWidget(group);
    }
}

/************************************************

 ************************************************/
auto LXQtTaskBar::removeWindow(windowMap_t::iterator pos) -> windowMap_t::iterator
{
    LXQtTaskButton * const group = *pos;
    auto ret = mKnownWindows.erase(pos);
    group->deleteLater();
    return ret;
}

/************************************************

 ************************************************/
void LXQtTaskBar::onActiveWindowChanged(WId window)
{
    LXQtTaskButton *button = mKnownWindows.value(window, nullptr);

    for (LXQtTaskButton *btn : qAsConst(mKnownWindows))
    {
        if(btn != button)
            btn->setChecked(false);
    }

    if (button)
        button->setChecked(true);
}

/************************************************

 ************************************************/
void LXQtTaskBar::onWindowChanged(WId window, NET::Properties prop, NET::Properties2 prop2)
{
    if(prop.testFlag(NET::WMWindowType) || prop.testFlag(NET::WMState) || prop2.testFlag(NET::WM2TransientFor))
    {
        if(acceptWindow(window))
            addWindow(window);
        else
            onWindowRemoved(window);
    }

    auto button = mKnownWindows.value(window);
    if(button == nullptr)
        return;

    if (prop.testFlag(NET::WMVisibleName) || prop.testFlag(NET::WMName))
        button->updateText();
    if (prop.testFlag(NET::WMIcon))
        button->updateIcon();
}

/************************************************

 ************************************************/
void LXQtTaskBar::onWindowAdded(WId window)
{
    auto const pos = mKnownWindows.find(window);
    if (mKnownWindows.end() == pos && acceptWindow(window))
        addWindow(window);
}

/************************************************

 ************************************************/
void LXQtTaskBar::onWindowRemoved(WId window)
{
    auto const pos = mKnownWindows.find(window);
    if (mKnownWindows.end() != pos)
    {
        removeWindow(pos);
    }
}
