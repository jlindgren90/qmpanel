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
#include "lxqttaskgroup.h"

using namespace LXQt;

/************************************************

************************************************/
LXQtTaskBar::LXQtTaskBar(Plugin *plugin, QWidget *parent) :
    QFrame(parent),
    mSignalMapper(new QSignalMapper(this)),
    mButtonStyle(Qt::ToolButtonTextBesideIcon),
    mButtonWidth(400),
    mButtonHeight(100),
    mCloseOnMiddleClick(true),
    mRaiseOnCurrentDesktop(true),
    mShowOnlyOneDesktopTasks(false),
    mShowDesktopNum(0),
    mShowOnlyCurrentScreenTasks(false),
    mShowOnlyMinimizedTasks(false),
    mAutoRotate(true),
    mGroupingEnabled(true),
    mShowGroupOnHover(true),
    mIconByClass(false),
    mCycleOnWheelScroll(true),
    mPlugin(plugin),
    mStyle(new LeftAlignedTextStyle())
{
    setStyle(mStyle);
    mLayout = new LXQt::GridLayout(this);
    setLayout(mLayout);
    mLayout->setMargin(0);
    mLayout->setStretch(LXQt::GridLayout::StretchHorizontal | LXQt::GridLayout::StretchVertical);

    setAcceptDrops(true);
    settingsChanged();
    realign();

    connect(mSignalMapper, static_cast<void (QSignalMapper::*)(int)>(&QSignalMapper::mapped), this, &LXQtTaskBar::activateTask);

    connect(KWindowSystem::self(), static_cast<void (KWindowSystem::*)(WId, NET::Properties, NET::Properties2)>(&KWindowSystem::windowChanged)
            , this, &LXQtTaskBar::onWindowChanged);
    connect(KWindowSystem::self(), &KWindowSystem::windowAdded, this, &LXQtTaskBar::onWindowAdded);
    connect(KWindowSystem::self(), &KWindowSystem::windowRemoved, this, &LXQtTaskBar::onWindowRemoved);
}

/************************************************

 ************************************************/
LXQtTaskBar::~LXQtTaskBar()
{
    delete mStyle;
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
void LXQtTaskBar::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasFormat(LXQtTaskGroup::mimeDataFormat()))
    {
        event->acceptProposedAction();
        buttonMove(nullptr, qobject_cast<LXQtTaskGroup *>(event->source()), event->pos());
    } else
        event->ignore();
    QWidget::dragEnterEvent(event);
}

/************************************************

 ************************************************/
void LXQtTaskBar::dragMoveEvent(QDragMoveEvent * event)
{
    //we don't get any dragMoveEvents if dragEnter wasn't accepted
    buttonMove(nullptr, qobject_cast<LXQtTaskGroup *>(event->source()), event->pos());
    QWidget::dragMoveEvent(event);
}

/************************************************

 ************************************************/
void LXQtTaskBar::buttonMove(LXQtTaskGroup * dst, LXQtTaskGroup * src, QPoint const & pos)
{
    int src_index;
    if (!src || -1 == (src_index = mLayout->indexOf(src)))
    {
        qDebug() << "Dropped invalid";
        return;
    }

    const int size = mLayout->count();
    Q_ASSERT(0 < size);
    //dst is nullptr in case the drop occured on empty space in taskbar
    int dst_index;
    if (nullptr == dst)
    {
        //moving based on taskbar (not signaled by button)
        QRect occupied = mLayout->occupiedGeometry();
        QRect last_empty_row{occupied};
        const QRect last_item_geometry = mLayout->itemAt(size - 1)->geometry();
        if (isRightToLeft())
        {
            last_empty_row.setTopRight(last_item_geometry.topLeft());
        } else
        {
            last_empty_row.setTopLeft(last_item_geometry.topRight());
        }
        if (occupied.contains(pos) && !last_empty_row.contains(pos))
            return;

        dst_index = size;
    } else
    {
        //moving based on signal from child button
        dst_index = mLayout->indexOf(dst);
    }

    //moving lower index to higher one => consider as the QList::move => insert(to, takeAt(from))
    if (src_index < dst_index)
    {
        if (size == dst_index
                || src_index + 1 != dst_index)
        {
            --dst_index;
        } else
        {
            //switching positions of next standing
            const int tmp_index = src_index;
            src_index = dst_index;
            dst_index = tmp_index;
        }
    }

    if (dst_index == src_index
            || mLayout->animatedMoveInProgress()
       )
        return;

    mLayout->moveItem(src_index, dst_index, true);
}

/************************************************

 ************************************************/
void LXQtTaskBar::groupBecomeEmptySlot()
{
    //group now contains no buttons - clean up in hash and delete the group
    LXQtTaskGroup * const group = qobject_cast<LXQtTaskGroup*>(sender());
    Q_ASSERT(group);

    for (auto i = mKnownWindows.begin(); mKnownWindows.end() != i; )
    {
        if (group == *i)
            i = mKnownWindows.erase(i);
        else
            ++i;
    }
    mLayout->removeWidget(group);
    group->deleteLater();
}

/************************************************

 ************************************************/
void LXQtTaskBar::addWindow(WId window)
{
    // If grouping disabled group behaves like regular button
    const QString group_id = mGroupingEnabled ? KWindowInfo(window, 0, NET::WM2WindowClass).windowClassClass() : QString("%1").arg(window);

    LXQtTaskGroup *group = nullptr;
    auto i_group = mKnownWindows.find(window);
    if (mKnownWindows.end() != i_group)
    {
        if ((*i_group)->groupName() == group_id)
            group = *i_group;
        else
            (*i_group)->onWindowRemoved(window);
    }

    //check if window belongs to some existing group
    if (!group && mGroupingEnabled)
    {
        for (auto i = mKnownWindows.cbegin(), i_e = mKnownWindows.cend(); i != i_e; ++i)
        {
            if ((*i)->groupName() == group_id)
            {
                group = *i;
                break;
            }
        }
    }

    if (!group)
    {
        group = new LXQtTaskGroup(group_id, window, this);
        connect(group, SIGNAL(groupBecomeEmpty(QString)), this, SLOT(groupBecomeEmptySlot()));
        connect(group, &LXQtTaskGroup::popupShown, this, &LXQtTaskBar::popupShown);
        connect(group, &LXQtTaskButton::dragging, this, [this] (QObject * dragSource, QPoint const & pos) {
            buttonMove(qobject_cast<LXQtTaskGroup *>(sender()), qobject_cast<LXQtTaskGroup *>(dragSource), pos);
        });

        mLayout->addWidget(group);
        group->setToolButtonsStyle(mButtonStyle);
    }

    mKnownWindows[window] = group;
    group->addWindow(window);
}

/************************************************

 ************************************************/
auto LXQtTaskBar::removeWindow(windowMap_t::iterator pos) -> windowMap_t::iterator
{
    WId const window = pos.key();
    LXQtTaskGroup * const group = *pos;
    auto ret = mKnownWindows.erase(pos);
    group->onWindowRemoved(window);
    return ret;
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
void LXQtTaskBar::onWindowChanged(WId window, NET::Properties prop, NET::Properties2 prop2)
{
    auto i = mKnownWindows.find(window);
    if (mKnownWindows.end() != i)
    {
        if (!(*i)->onWindowChanged(window, prop, prop2) && acceptWindow(window))
        { // window is removed from a group because of class change, so we should add it again
            addWindow(window);
        }
    }
}

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
void LXQtTaskBar::setButtonStyle(Qt::ToolButtonStyle buttonStyle)
{
    const Qt::ToolButtonStyle old_style = mButtonStyle;
    mButtonStyle = buttonStyle;
    if (old_style != mButtonStyle)
        emit buttonStyleRefreshed(mButtonStyle);
}

/************************************************

 ************************************************/
void LXQtTaskBar::settingsChanged()
{
    bool groupingEnabledOld = mGroupingEnabled;
    bool showOnlyOneDesktopTasksOld = mShowOnlyOneDesktopTasks;
    const int showDesktopNumOld = mShowDesktopNum;
    bool showOnlyCurrentScreenTasksOld = mShowOnlyCurrentScreenTasks;
    bool showOnlyMinimizedTasksOld = mShowOnlyMinimizedTasks;
    const bool iconByClassOld = mIconByClass;

    mButtonWidth = 200; /* TODO: scale by DPI */
    mButtonHeight = 100;

    setButtonStyle(Qt::ToolButtonTextBesideIcon);

    mShowOnlyOneDesktopTasks = false;
    mShowDesktopNum = 0;
    mShowOnlyCurrentScreenTasks = false;
    mShowOnlyMinimizedTasks = false;
    mAutoRotate = true;
    mCloseOnMiddleClick = true;
    mRaiseOnCurrentDesktop = false;
    mGroupingEnabled = false;
    mShowGroupOnHover = true;
    mIconByClass = false;
    mCycleOnWheelScroll = true;

    // Delete all groups if grouping feature toggled and start over
    if (groupingEnabledOld != mGroupingEnabled)
    {
        for (int i = mLayout->count() - 1; 0 <= i; --i)
        {
            LXQtTaskGroup * group = qobject_cast<LXQtTaskGroup*>(mLayout->itemAt(i)->widget());
            if (nullptr != group)
            {
                mLayout->takeAt(i);
                group->deleteLater();
            }
        }
        mKnownWindows.clear();
    }

    if (showOnlyOneDesktopTasksOld != mShowOnlyOneDesktopTasks
            || (mShowOnlyOneDesktopTasks && showDesktopNumOld != mShowDesktopNum)
            || showOnlyCurrentScreenTasksOld != mShowOnlyCurrentScreenTasks
            || showOnlyMinimizedTasksOld != mShowOnlyMinimizedTasks
            )
        emit showOnlySettingChanged();
    if (iconByClassOld != mIconByClass)
        emit iconByClassChanged();

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

    //our placement on screen could have been changed
    emit showOnlySettingChanged();
    emit refreshIconGeometry();
}

/************************************************

 ************************************************/
void LXQtTaskBar::wheelEvent(QWheelEvent* event)
{
    if (!mCycleOnWheelScroll)
        return QFrame::wheelEvent(event);

    static int threshold = 0;
    threshold += abs(event->delta());
    if (threshold < 300)
        return QFrame::wheelEvent(event);
    else
        threshold = 0;

    int delta = event->delta() < 0 ? 1 : -1;

    // create temporary list of visible groups in the same order like on the layout
    QList<LXQtTaskGroup*> list;
    LXQtTaskGroup *group = NULL;
    for (int i = 0; i < mLayout->count(); i++)
    {
        QWidget * o = mLayout->itemAt(i)->widget();
        LXQtTaskGroup * g = qobject_cast<LXQtTaskGroup *>(o);
        if (!g)
            continue;

        if (g->isVisible())
            list.append(g);
        if (g->isChecked())
            group = g;
    }

    if (list.isEmpty())
        return QFrame::wheelEvent(event);

    if (!group)
        group = list.at(0);

    LXQtTaskButton *button = NULL;

    // switching between groups from temporary list in modulo addressing
    while (!button)
    {
        button = group->getNextPrevChildButton(delta == 1, !(list.count() - 1));
        if (button)
            button->raiseApplication();
        int idx = (list.indexOf(group) + delta + list.count()) % list.count();
        group = list.at(idx);
    }
    QFrame::wheelEvent(event);
}

/************************************************

 ************************************************/
void LXQtTaskBar::resizeEvent(QResizeEvent* event)
{
    emit refreshIconGeometry();
    return QWidget::resizeEvent(event);
}

/************************************************

 ************************************************/
void LXQtTaskBar::changeEvent(QEvent* event)
{
    // if current style is changed, reset the base style of the proxy style
    // so we can apply the new style correctly to task buttons.
    if(event->type() == QEvent::StyleChange)
        mStyle->setBaseStyle(NULL);

    QFrame::changeEvent(event);
}

/************************************************

 ************************************************/
void LXQtTaskBar::activateTask(int pos)
{
    for (int i = 1; i < mLayout->count(); ++i)
    {
        QWidget * o = mLayout->itemAt(i)->widget();
        LXQtTaskGroup * g = qobject_cast<LXQtTaskGroup *>(o);
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
