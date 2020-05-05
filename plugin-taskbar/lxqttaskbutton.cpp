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

#include "lxqttaskbutton.h"

#include <KWindowSystem>
#include <NETWM>
#include <QDragEnterEvent>
#include <QStyle>
#include <QTimer>
#include <QX11Info>

LXQtTaskButton::LXQtTaskButton(const WId window, QWidget * parent)
    : QToolButton(parent), mWindow(window)
{
    setCheckable(true);
    setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    setAcceptDrops(true);

    updateText();
    updateIcon();

    if (KWindowSystem::activeWindow() == window)
        setChecked(true);

    mTimer.setSingleShot(true);
    mTimer.setInterval(500);

    connect(&mTimer, &QTimer::timeout, [window]() {
        KWindowSystem::forceActiveWindow(window);
        xcb_flush(QX11Info::connection());
    });
}

void LXQtTaskButton::updateText()
{
    KWindowInfo info(mWindow, NET::WMVisibleName | NET::WMName);
    QString title = info.visibleName();
    if (title.isEmpty())
        title = info.name();

    setText(title.replace("&", "&&"));
    setToolTip(title);
}

void LXQtTaskButton::updateIcon()
{
    int size = style()->pixelMetric(QStyle::PM_ToolBarIconSize);
    size *= devicePixelRatioF();
    setIcon(KWindowSystem::icon(mWindow, size, size));
}

QSize LXQtTaskButton::sizeHint() const
{
    return {200, /* TODO: scale with DPI */
            QToolButton::sizeHint().height()};
}

void LXQtTaskButton::dragEnterEvent(QDragEnterEvent * event)
{
    mTimer.start();
    event->acceptProposedAction();
    QToolButton::dragEnterEvent(event);
}

void LXQtTaskButton::dragLeaveEvent(QDragLeaveEvent * event)
{
    mTimer.stop();
    QToolButton::dragLeaveEvent(event);
}

void LXQtTaskButton::dropEvent(QDropEvent * event)
{
    mTimer.stop();
    QToolButton::dropEvent(event);
}

void LXQtTaskButton::mousePressEvent(QMouseEvent * event)
{
    if (event->button() == Qt::LeftButton)
    {
        if (isChecked())
            KWindowSystem::minimizeWindow(mWindow);
        else
            KWindowSystem::forceActiveWindow(mWindow);
    }
    else if (event->button() == Qt::MiddleButton)
    {
        NETRootInfo info(QX11Info::connection(), NET::CloseWindow);
        info.closeWindowRequest(mWindow);
    }

    event->accept();
}
