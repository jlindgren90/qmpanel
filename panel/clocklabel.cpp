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

ClockLabel::ClockLabel(QWidget * parent)
    : QToolButton(parent), mCalendarAction(this)
{
    mCalendarAction.setDefaultWidget(&mCalendar);
    mMenu.addAction(&mCalendarAction);

    setAutoRaise(true);
    setMenu(&mMenu);
    setPopupMode(InstantPopup);
    setStyleSheet("QToolButton::menu-indicator { image: none; }");

    startTimer(10000);
    timerEvent(nullptr);

    connect(&mMenu, &QMenu::aboutToShow,
            [this]() { mCalendar.setSelectedDate(QDate::currentDate()); });
}

void ClockLabel::timerEvent(QTimerEvent *)
{
    setText(QDateTime::currentDateTime().toString("ddd MMM d, h:mm a"));
}
