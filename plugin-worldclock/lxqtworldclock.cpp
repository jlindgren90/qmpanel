/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2012-2013 Razor team
 *            2014 LXQt team
 * Authors:
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

#include "lxqtworldclock.h"

#include <QCalendarWidget>
#include <QMouseEvent>

class ClockLabel : public QLabel
{
public:
    ClockLabel(Plugin * plugin, QWidget * parent)
        : QLabel(parent), mPlugin(plugin)
    {
        mCalendar.setWindowFlags(Qt::Popup);
    }

protected:
    void mousePressEvent(QMouseEvent * e) override;

private:
    Plugin * const mPlugin;
    QCalendarWidget mCalendar;
};

void ClockLabel::mousePressEvent(QMouseEvent * e)
{
    if (e->button() == Qt::LeftButton)
    {
        if (mCalendar.isVisible())
            mCalendar.hide();
        else
        {
            auto size = mCalendar.sizeHint();
            mCalendar.move(mPlugin->calculatePopupWindowPos(size).topLeft());
            mCalendar.show();
        }

        e->accept();
        return;
    }

    QLabel::mousePressEvent(e);
}

LXQtWorldClock::LXQtWorldClock(LXQtPanel * lxqtPanel)
    : Plugin(lxqtPanel), mLabel(new ClockLabel(this, lxqtPanel))
{
    mTimer.setInterval(10000);
    mTimer.start();

    QObject::connect(&mTimer, &QTimer::timeout, [this]() { updateLabel(); });

    updateLabel();
}

void LXQtWorldClock::updateLabel()
{
    mLabel->setText(QDateTime::currentDateTime().toString("ddd MMM d, h:mm a"));
}
