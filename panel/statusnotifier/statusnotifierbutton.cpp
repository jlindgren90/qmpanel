/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * qmpanel - a minimal Qt-based desktop panel
 *
 * Copyright: 2015 LXQt team
 * Copyright: 2023 John Lindgren
 * Authors:
 *  Balázs Béla <balazsbela[at]gmail.com>
 *  Paulo Lieuthier <paulolieuthier@gmail.com>
 *  Palo Kisa <palo.kisa@gmail.com>
 *  John Lindgren <john@jlindgren.net>
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

#include <QMenu>
#include <QMouseEvent>
#include <dbusmenu-qt5/dbusmenuimporter.h>

StatusNotifierButton::StatusNotifierButton(QString service, QString objectPath,
                                           QWidget * parent)
    : QToolButton(parent),
      mSni(service, objectPath, QDBusConnection::sessionBus())
{
    setAutoRaise(true);

    connect(&mSni, &org::kde::StatusNotifierItem::NewIcon, this,
            &StatusNotifierButton::newIcon);
    connect(&mSni, &org::kde::StatusNotifierItem::NewToolTip, this,
            &StatusNotifierButton::newToolTip);

    getPropertyAsync("Menu", [this](QVariant value) {
        auto path = qdbus_cast<QDBusObjectPath>(value);
        if (!path.path().isEmpty())
        {
            auto importer =
                new DBusMenuImporter(mSni.service(), path.path(), this);
            mMenu = importer->menu();
        }
    });

    newIcon();
    newToolTip();
}

void StatusNotifierButton::getPropertyAsync(
    QString const & name, std::function<void(QVariant)> finished)
{
    auto msg = QDBusMessage::createMethodCall(
        mSni.service(), mSni.path(), "org.freedesktop.DBus.Properties", "Get");
    msg << mSni.interface() << name;
    auto call = mSni.connection().asyncCall(msg);

    connect(new QDBusPendingCallWatcher(call, this),
            &QDBusPendingCallWatcher::finished,
            [this, finished](QDBusPendingCallWatcher * cw) {
                QDBusPendingReply<QVariant> reply = *cw;
                if (reply.isError())
                    qDebug() << "Error on DBus request(" << mSni.service()
                             << ',' << mSni.path() << "): " << reply.error();
                finished(reply.value());
                cw->deleteLater();
            });
}

void StatusNotifierButton::newIcon()
{
    getPropertyAsync("IconName", [this](QVariant value) {
        auto iconName = qdbus_cast<QString>(value);
        setIcon(QIcon::fromTheme(iconName));
    });
}

void StatusNotifierButton::newToolTip()
{
    getPropertyAsync("ToolTip", [this](QVariant value) {
        auto tooltip = qdbus_cast<ToolTip>(value);
        setToolTip(tooltip.title);
    });
}

void StatusNotifierButton::mouseReleaseEvent(QMouseEvent * event)
{
    auto pos = mapToGlobal(QPoint()); // left top corner

    if (event->button() == Qt::LeftButton)
        mSni.Activate(pos.x(), pos.y());
    else if (event->button() == Qt::MiddleButton)
        mSni.SecondaryActivate(pos.x(), pos.y());
    else if (Qt::RightButton == event->button())
    {
        if (mMenu)
            mMenu->popup(pos);
        else
            mSni.ContextMenu(pos.x(), pos.y());
    }

    QToolButton::mouseReleaseEvent(event);
}
