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

#include <QApplication>
#include <QDebug>
#include <QSignalMapper>
#include <QToolButton>
#include <QSettings>
#include <QList>
#include <QMimeData>
#include <QWheelEvent>
#include <QFlag>
#include <QX11Info>
#include <QDebug>
#include <QTimer>

#include <lxqt-globalkeys.h>
#include <LXQt/GridLayout>
#include <XdgIcon>

#include "lxqttaskbar.h"

using namespace LXQt;

/************************************************

************************************************/
LXQtTaskBar::LXQtTaskBar(Plugin *plugin, QWidget *parent) :
    QFrame(parent),
    mSignalMapper(new QSignalMapper(this)),
    mButtonWidth(400),
    mButtonHeight(100),
    mPlugin(plugin)
{
    mLayout = new LXQt::GridLayout(this);
    setLayout(mLayout);
    mLayout->setMargin(0);
    mLayout->setStretch(LXQt::GridLayout::StretchHorizontal | LXQt::GridLayout::StretchVertical);

    setAcceptDrops(true);
    settingsChanged();
    realign();

    connect(mSignalMapper, static_cast<void (QSignalMapper::*)(int)>(&QSignalMapper::mapped), this, &LXQtTaskBar::activateTask);

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

    info = KWindowInfo(transFor, NET::WMWindowType);

    QFlags<NET::WindowTypeMask> normalFlag;
    normalFlag |= NET::NormalMask;
    normalFlag |= NET::DialogMask;
    normalFlag |= NET::UtilityMask;

    return !NET::typeMatchesMask(info.windowType(NET::AllTypesMask), normalFlag);
}

/************************************************

 ************************************************/
void LXQtTaskBar::addWindow(WId window)
{
    if (mKnownWindows.find(window) == mKnownWindows.end())
    {
        auto group = new LXQtTaskButton(window, this, this);
        mKnownWindows[window] = group;
        mLayout->addWidget(group);
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
    {
        button->setChecked(true);
        if (button->hasUrgencyHint())
            button->setUrgencyHint(false);
    }
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

    if (prop.testFlag(NET::WMState))
    {
        KWindowInfo info{window, NET::WMState};
        button->setUrgencyHint(info.hasState(NET::DemandsAttention));
    }
}

/************************************************

 ************************************************/
void LXQtTaskBar::refreshTaskList()
{
    QList<WId> new_list;
    // Just add new windows to groups, deleting is up to the groups
    const auto wnds = KWindowSystem::stackingOrder();
    for (auto const wnd: wnds)
    {
        if (acceptWindow(wnd))
        {
            new_list << wnd;
            addWindow(wnd);
        }
    }

    //emulate windowRemoved if known window not reported by KWindowSystem
    for (auto i = mKnownWindows.begin(), i_e = mKnownWindows.end(); i != i_e; )
    {
        if (0 > new_list.indexOf(i.key()))
        {
            i = removeWindow(i);
        } else
            ++i;
    }
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

/************************************************

 ************************************************/
void LXQtTaskBar::settingsChanged()
{
    mButtonWidth = 200; /* TODO: scale by DPI */
    mButtonHeight = 100;

    refreshTaskList();
}

/************************************************

 ************************************************/
void LXQtTaskBar::realign()
{
    mLayout->setEnabled(false);

    QSize maxSize = QSize(mButtonWidth, mButtonHeight);
    QSize minSize = QSize(0, 0);

    mLayout->setRowCount(1);
    mLayout->setColumnCount(0);

    mLayout->setCellMinimumSize(minSize);
    mLayout->setCellMaximumSize(maxSize);
    mLayout->setDirection(LXQt::GridLayout::LeftToRight);
    mLayout->setEnabled(true);
}

/************************************************

 ************************************************/
void LXQtTaskBar::activateTask(int pos)
{
    for (int i = 1; i < mLayout->count(); ++i)
    {
        QWidget * o = mLayout->itemAt(i)->widget();
        LXQtTaskButton * g = qobject_cast<LXQtTaskButton *>(o);
        if (g && g->isVisible())
        {
            pos--;
            if (pos == 0)
            {
                g->raiseApplication();
                break;
            }
        }
    }
}
