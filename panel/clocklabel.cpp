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
#include "mainpanel.h"

#include <QMouseEvent>

ClockLabel::ClockLabel(MainPanel * panel) : QLabel(panel), mPanel(panel)
{
    mCalendar.setWindowFlags(Qt::Popup);
    mTimer.setInterval(10000);
    mTimer.start();

    QObject::connect(&mTimer, &QTimer::timeout, this, &ClockLabel::updateTime);

    updateTime();
}

void ClockLabel::mousePressEvent(QMouseEvent * e)
{
    if (e->button() == Qt::LeftButton)
    {
        if (mCalendar.isVisible())
            mCalendar.hide();
        else
        {
            auto size = mCalendar.sizeHint();
            mCalendar.move(mPanel->calcPopupPos(this, size).topLeft());
            mCalendar.show();
        }

        e->accept();
        return;
    }

    QLabel::mousePressEvent(e);
}

void ClockLabel::updateTime()
{
    setText(QDateTime::currentDateTime().toString("ddd MMM d, h:mm a"));
}
