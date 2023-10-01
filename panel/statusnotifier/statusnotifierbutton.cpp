/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2015 LXQt team
 * Authors:
 *  Balázs Béla <balazsbela[at]gmail.com>
 *  Paulo Lieuthier <paulolieuthier@gmail.com>
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
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 *
 * END_COMMON_COPYRIGHT_HEADER */

#include "statusnotifierbutton.h"
#include "sniasync.h"

#include <QMenu>
#include <QMouseEvent>
#include <dbusmenu-qt5/dbusmenuimporter.h>

StatusNotifierButton::StatusNotifierButton(QString service, QString objectPath,
                                           QWidget * parent)
    : QToolButton(parent), mMenu(nullptr)
{
    setAutoRaise(true);
    interface =
        new SniAsync(service, objectPath, QDBusConnection::sessionBus(), this);

    connect(interface, &SniAsync::NewIcon, this,
            &StatusNotifierButton::newIcon);
    connect(interface, &SniAsync::NewToolTip, this,
            &StatusNotifierButton::newToolTip);

    interface->propertyGetAsync("Menu", [this](QDBusObjectPath path) {
        if (!path.path().isEmpty())
        {
            auto importer =
                new DBusMenuImporter(interface->service(), path.path(), this);
            mMenu = importer->menu();
        }
    });

    newIcon();
    newToolTip();
}

StatusNotifierButton::~StatusNotifierButton() { delete interface; }

void StatusNotifierButton::newIcon()
{
    interface->propertyGetAsync("IconName", [this](QString iconName) {
        setIcon(QIcon::fromTheme(iconName));
    });
}

void StatusNotifierButton::newToolTip()
{
    interface->propertyGetAsync(
        "ToolTip", [this](ToolTip tooltip) { setToolTip(tooltip.title); });
}

void StatusNotifierButton::mouseReleaseEvent(QMouseEvent * event)
{
    if (event->button() == Qt::LeftButton)
        interface->Activate(QCursor::pos().x(), QCursor::pos().y());
    else if (event->button() == Qt::MiddleButton)
        interface->SecondaryActivate(QCursor::pos().x(), QCursor::pos().y());
    else if (Qt::RightButton == event->button())
    {
        if (mMenu)
            mMenu->popup(event->globalPos());
        else
            interface->ContextMenu(QCursor::pos().x(), QCursor::pos().y());
    }

    QToolButton::mouseReleaseEvent(event);
}
