/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * qmpanel - a minimal Qt-based desktop panel
 *
 * Copyright: 2012-2013 Razor team
 *            2014 LXQt team
 *            2020 John Lindgren
 * Authors:
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

#include "clocklabel.h"
#include <LayerShellQt/shell.h>
#include <QtWaylandClient/private/qwayland-xdg-shell.h>

ClockLabel::ClockLabel(QWidget * parent)
    : QToolButton(parent), mCalendarAction(this)
{
    mCalendarAction.setDefaultWidget(&mCalendar);
    mMenu.addAction(&mCalendarAction);

    mMenu.show();
    mMenu.windowHandle()->setProperty(
        "_q_waylandPopupAnchor", QVariant::fromValue(Qt::Edge::BottomEdge));
    mMenu.windowHandle()->setProperty(
        "_q_waylandPopupGravity", QVariant::fromValue(Qt::Edge::BottomEdge));
    mMenu.windowHandle()->setProperty(
        "_q_waylandPopupConstraintAdjustment",
        static_cast<unsigned int>(
            QtWayland::xdg_positioner::constraint_adjustment_slide_x |
            QtWayland::xdg_positioner::constraint_adjustment_flip_y));
    mMenu.hide();

    setAutoRaise(true);
    setMenu(&mMenu);
    setPopupMode(InstantPopup);
    setStyleSheet("QToolButton::menu-indicator { image: none; }");

    startTimer(10000);
    timerEvent(nullptr);

    connect(&mMenu, &QMenu::aboutToShow, [this]() {
        mMenu.windowHandle()->setProperty("_q_waylandPopupAnchorRect",
                                          geometry());
        mCalendar.setSelectedDate(QDate::currentDate());
    });
}

void ClockLabel::timerEvent(QTimerEvent *)
{
    setText(QDateTime::currentDateTime().toString("ddd MMM d, h:mm a"));
}
