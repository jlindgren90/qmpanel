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

#include "lxqttaskgroup.h"
#include "lxqttaskbar.h"

#include <QDebug>
#include <QMimeData>
#include <QFocusEvent>
#include <QDragLeaveEvent>
#include <QStringBuilder>
#include <QMenu>
#include <XdgIcon>
#include <KF5/KWindowSystem/KWindowSystem>
#include <functional>

/************************************************

 ************************************************/
LXQtTaskGroup::LXQtTaskGroup(const QString &groupName, WId window, LXQtTaskBar *parent)
    : LXQtTaskButton(window, parent, parent),
    mGroupName(groupName)
{
    Q_ASSERT(parent);

    setObjectName(groupName);
    setText(groupName);

    connect(KWindowSystem::self(), SIGNAL(currentDesktopChanged(int)), this, SLOT(onDesktopChanged(int)));
    connect(KWindowSystem::self(), SIGNAL(activeWindowChanged(WId)), this, SLOT(onActiveWindowChanged(WId)));
    connect(parent, &LXQtTaskBar::showOnlySettingChanged, this, &LXQtTaskGroup::refreshVisibility);
}

/************************************************

 ************************************************/
void LXQtTaskGroup::closeGroup()
{
    for (LXQtTaskButton *button : qAsConst(mButtonHash) )
        if (button->isOnDesktop(KWindowSystem::currentDesktop()))
            button->closeApplication();
}

/************************************************

 ************************************************/
LXQtTaskButton * LXQtTaskGroup::addWindow(WId id)
{
    if (mButtonHash.contains(id))
        return mButtonHash.value(id);

    LXQtTaskButton *btn = new LXQtTaskButton(id, parentTaskBar(), this);

    if (btn->isApplicationActive())
    {
        btn->setChecked(true);
        setChecked(true);
    }

    mButtonHash.insert(id, btn);

    refreshVisibility();

    return btn;
}

/************************************************

 ************************************************/
LXQtTaskButton * LXQtTaskGroup::checkedButton() const
{
    for (LXQtTaskButton* button : qAsConst(mButtonHash))
        if (button->isChecked())
            return button;

    return NULL;
}

/************************************************

 ************************************************/
void LXQtTaskGroup::onActiveWindowChanged(WId window)
{
    LXQtTaskButton *button = mButtonHash.value(window, nullptr);
    for (LXQtTaskButton *btn : qAsConst(mButtonHash))
        btn->setChecked(false);

    if (button)
    {
        button->setChecked(true);
        if (button->hasUrgencyHint())
            button->setUrgencyHint(false);
    }
    setChecked(nullptr != button);
}

/************************************************

 ************************************************/
void LXQtTaskGroup::onDesktopChanged(int number)
{
    refreshVisibility();
}

/************************************************

 ************************************************/
void LXQtTaskGroup::onWindowRemoved(WId window)
{
    if (mButtonHash.contains(window))
    {
        LXQtTaskButton *button = mButtonHash.value(window);
        mButtonHash.remove(window);
        button->deleteLater();

        if (mButtonHash.count())
            regroup();
        else
        {
            if (isVisible())
                emit visibilityChanged(false);
            hide();
            emit groupBecomeEmpty(groupName());

        }
    }
}

/************************************************

 ************************************************/
int LXQtTaskGroup::buttonsCount() const
{
    return mButtonHash.count();
}

/************************************************

 ************************************************/
int LXQtTaskGroup::visibleButtonsCount() const
{
    int i = 0;
    for (LXQtTaskButton *btn : qAsConst(mButtonHash))
        i++;
    return i;
}

/************************************************

 ************************************************/
void LXQtTaskGroup::regroup()
{
    int cont = visibleButtonsCount();

    if (cont == 1)
    {
        // Get first visible button
        LXQtTaskButton * button = NULL;
        for (LXQtTaskButton *btn : qAsConst(mButtonHash))
        {
            button = btn;
            break;
        }

        if (button)
        {
            setText(button->text());
            setToolTip(button->toolTip());
            setWindowId(button->windowId());
        }
    }
    else if (cont == 0)
        hide();
    else
        qWarning("should not be reached\n");
}

/************************************************

 ************************************************/
void LXQtTaskGroup::refreshVisibility()
{
    bool will = false;
    LXQtTaskBar const * taskbar = parentTaskBar();
    const int showDesktop = taskbar->showDesktopNum();
    for(LXQtTaskButton * btn : qAsConst(mButtonHash))
    {
        bool visible = taskbar->isShowOnlyOneDesktopTasks() ? btn->isOnDesktop(0 == showDesktop ? KWindowSystem::currentDesktop() : showDesktop) : true;
        visible &= taskbar->isShowOnlyCurrentScreenTasks() ? btn->isOnCurrentScreen() : true;
        visible &= taskbar->isShowOnlyMinimizedTasks() ? btn->isMinimized() : true;
        btn->setVisible(visible);
        will |= visible;
    }

    bool is = isVisible();
    setVisible(will);
    regroup();

    if (is != will)
        emit visibilityChanged(will);
}

/************************************************

 ************************************************/
QMimeData * LXQtTaskGroup::mimeData()
{
    QMimeData *mimedata = new QMimeData;
    QByteArray byteArray;
    QDataStream stream(&byteArray, QIODevice::WriteOnly);
    stream << groupName();
    mimedata->setData(mimeDataFormat(), byteArray);
    return mimedata;
}

/************************************************

 ************************************************/
bool LXQtTaskGroup::onWindowChanged(WId window, NET::Properties prop, NET::Properties2 prop2)
{ // returns true if the class is preserved
    bool needsRefreshVisibility{false};
    QVector<LXQtTaskButton *> buttons;
    if (mButtonHash.contains(window))
        buttons.append(mButtonHash.value(window));

    // If group is based on that window properties must be changed also on button group
    if (window == windowId())
        buttons.append(this);

    if (!buttons.isEmpty())
    {
        // window changed virtual desktop
        if (prop.testFlag(NET::WMDesktop) || prop.testFlag(NET::WMGeometry))
        {
            if (parentTaskBar()->isShowOnlyOneDesktopTasks()
                    || parentTaskBar()->isShowOnlyCurrentScreenTasks())
            {
                needsRefreshVisibility = true;
            }
        }

        if (prop.testFlag(NET::WMVisibleName) || prop.testFlag(NET::WMName))
            std::for_each(buttons.begin(), buttons.end(), std::mem_fn(&LXQtTaskButton::updateText));

        // XXX: we are setting window icon geometry -> don't need to handle NET::WMIconGeometry
        // Icon of the button can be based on windowClass
        if (prop.testFlag(NET::WMIcon) || prop2.testFlag(NET::WM2WindowClass))
            std::for_each(buttons.begin(), buttons.end(), std::mem_fn(&LXQtTaskButton::updateIcon));

        if (prop.testFlag(NET::WMState))
        {
            KWindowInfo info{window, NET::WMState};
            if (info.hasState(NET::SkipTaskbar))
                onWindowRemoved(window);
            std::for_each(buttons.begin(), buttons.end(), std::bind(&LXQtTaskButton::setUrgencyHint, std::placeholders::_1, info.hasState(NET::DemandsAttention)));

            if (parentTaskBar()->isShowOnlyMinimizedTasks())
            {
                needsRefreshVisibility = true;
            }
        }
    }

    if (needsRefreshVisibility)
        refreshVisibility();

    return true;
}
