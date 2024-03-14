/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * qmpanel - a minimal Qt-based desktop panel
 *
 * Copyright: 2011 Razor team
 *            2014 LXQt team
 *            2020-2024 John Lindgren
 * Authors:
 *   Alexander Sokoloff <sokoloff.a@gmail.com>
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

#include "taskbutton.h"

#include <KWindowInfo>
#include <KX11Extras>
#include <NETWM>
#include <QDragEnterEvent>
#include <QStyle>
#include <QTimer>
#include <private/qtx11extras_p.h>

TaskButton::TaskButton(QWidget * parent) : QToolButton(parent)
{
    setCheckable(true);
    setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    setAcceptDrops(true);

    mTimer.setSingleShot(true);
    mTimer.setInterval(500);

    connect(this, &QToolButton::clicked, [this](bool checked) {
        if (checked)
            activateWindow();
        else
            minimizeWindow();
    });

    connect(&mTimer, &QTimer::timeout, this, &TaskButton::activateWindow);
}

QSize TaskButton::sizeHint() const
{
    return {2 * logicalDpiX(), QToolButton::sizeHint().height()};
}

void TaskButton::dragEnterEvent(QDragEnterEvent * event)
{
    mTimer.start();
    event->acceptProposedAction();
    QToolButton::dragEnterEvent(event);
}

void TaskButton::dragLeaveEvent(QDragLeaveEvent * event)
{
    mTimer.stop();
    QToolButton::dragLeaveEvent(event);
}

void TaskButton::dropEvent(QDropEvent * event)
{
    mTimer.stop();
    QToolButton::dropEvent(event);
}

void TaskButton::mousePressEvent(QMouseEvent * event)
{
    if (event->button() == Qt::MiddleButton)
    {
        closeWindow();
        event->accept();
        return;
    }

    QToolButton::mousePressEvent(event);
}

TaskButtonX11::TaskButtonX11(const WId window, QWidget * parent)
    : TaskButton(parent), mWindow(window)
{
    updateText();
    updateIcon();

    if (KX11Extras::activeWindow() == window)
        setChecked(true);
}

void TaskButtonX11::updateText()
{
    KWindowInfo info(mWindow, NET::WMVisibleName | NET::WMName);
    QString title = info.visibleName();
    if (title.isEmpty())
        title = info.name();

    setText(title.replace("&", "&&"));
    setToolTip(title);
}

void TaskButtonX11::updateIcon()
{
    int size = style()->pixelMetric(QStyle::PM_ToolBarIconSize);
    size *= devicePixelRatioF();
    QIcon icon = KX11Extras::icon(mWindow, size, size);
    if (icon.isNull())
        icon = style()->standardIcon(QStyle::SP_FileIcon);
    setIcon(icon);
}

void TaskButtonX11::activateWindow()
{
    KX11Extras::forceActiveWindow(mWindow);
    // need to flush if called from timer
    xcb_flush(QX11Info::connection());
}

void TaskButtonX11::minimizeWindow() { KX11Extras::minimizeWindow(mWindow); }

void TaskButtonX11::closeWindow()
{
    NETRootInfo info(QX11Info::connection(), NET::CloseWindow);
    info.closeWindowRequest(mWindow);
}
