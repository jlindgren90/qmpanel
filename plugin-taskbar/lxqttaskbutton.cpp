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
#include "lxqttaskbar.h"

#include "../panel/lxqtpanel.h"

#include <KWindowSystem>
#include <QAction>
#include <QDragEnterEvent>
#include <QMenu>
#include <QStyle>
#include <QTimer>
#include <QX11Info>
#include <XdgIcon>

/************************************************

************************************************/
LXQtTaskButton::LXQtTaskButton(const WId window, LXQtPanel *panel, QWidget *parent) :
    QToolButton(parent),
    mWindow(window),
    mPanel(panel),
    mIconSize(style()->pixelMetric(QStyle::PM_ToolBarIconSize)),
    mDNDTimer(new QTimer(this))
{
    setCheckable(true);
    setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    setAcceptDrops(true);

    updateText();
    updateIcon();

    if(KWindowSystem::activeWindow() == mWindow)
        setChecked(true);

    mDNDTimer->setSingleShot(true);
    mDNDTimer->setInterval(700);
    connect(mDNDTimer, SIGNAL(timeout()), this, SLOT(activateWithDraggable()));
}

/************************************************

 ************************************************/
void LXQtTaskButton::updateText()
{
    KWindowInfo info(mWindow, NET::WMVisibleName | NET::WMName);
    QString title = info.visibleName().isEmpty() ? info.name() : info.visibleName();
    setText(title.replace("&", "&&"));
    setToolTip(title);
}

/************************************************

 ************************************************/
void LXQtTaskButton::updateIcon()
{
    int devicePixels = mIconSize * devicePixelRatioF();
    QIcon ico = KWindowSystem::icon(mWindow, devicePixels, devicePixels);
    setIcon(ico.isNull() ? XdgIcon::defaultApplicationIcon() : ico);
}

QSize LXQtTaskButton::sizeHint() const
{
    return {200, /* TODO: scale with DPI */
            QToolButton::sizeHint().height()};
}

/************************************************

 ************************************************/
void LXQtTaskButton::dragEnterEvent(QDragEnterEvent *event)
{
    // It must be here otherwise dragLeaveEvent and dragMoveEvent won't be called
    // on the other hand drop and dragmove events of parent widget won't be called
    event->acceptProposedAction();
    mDNDTimer->start();
    QToolButton::dragEnterEvent(event);
}

void LXQtTaskButton::dragLeaveEvent(QDragLeaveEvent *event)
{
    mDNDTimer->stop();
    QToolButton::dragLeaveEvent(event);
}

void LXQtTaskButton::dropEvent(QDropEvent *event)
{
    mDNDTimer->stop();
    QToolButton::dropEvent(event);
}

/************************************************

 ************************************************/
void LXQtTaskButton::mousePressEvent(QMouseEvent* event)
{
    if(event->button() == Qt::LeftButton)
    {
        if (isChecked())
            KWindowSystem::minimizeWindow(mWindow);
        else
            KWindowSystem::activateWindow(mWindow);
    }
    else if(event->button() == Qt::MiddleButton)
    {
        NETRootInfo(QX11Info::connection(), NET::CloseWindow).closeWindowRequest(mWindow);
    }

    event->accept();
}

/************************************************

 ************************************************/
void LXQtTaskButton::activateWithDraggable()
{
    // raise app in any time when there is a drag
    // in progress to allow drop it into an app
    KWindowSystem::activateWindow(mWindow);
    KWindowSystem::forceActiveWindow(mWindow);
}
