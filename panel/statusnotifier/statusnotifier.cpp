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

#include "statusnotifier.h"
#include "../mainpanel.h"
#include "statusnotifiericon.h"

#include <QBoxLayout>
#include <unistd.h>

StatusNotifier::StatusNotifier(MainPanel * panel)
    : QWidget(panel), mPanel(panel), mLayout(this)
{
    mLayout.setContentsMargins(QMargins());
    mLayout.setSpacing(logicalDpiX() / 24);

    QString dbusName =
        QStringLiteral("org.kde.StatusNotifierHost-%1-1").arg(getpid());

    if (QDBusConnection::sessionBus().interface()->registerService(
            dbusName, QDBusConnectionInterface::DontQueueService) ==
        QDBusConnectionInterface::ServiceNotRegistered)
    {
        qWarning() << "StatusNotifier: unable to register service for "
                   << dbusName;
    }

    mWatcher.RegisterStatusNotifierHost(dbusName);

    connect(&mWatcher, &StatusNotifierWatcher::StatusNotifierItemRegistered,
            this, &StatusNotifier::itemAdded);
    connect(&mWatcher, &StatusNotifierWatcher::StatusNotifierItemUnregistered,
            this, &StatusNotifier::itemRemoved);

    for (const auto & service : mWatcher.RegisteredStatusNotifierItems())
        itemAdded(service);
}

void StatusNotifier::registerMenu(QMenu * menu) { mPanel->registerMenu(menu); }

static void insertSorted(QBoxLayout * layout, QWidget * widget)
{
    int idx = 0;
    for (; idx < layout->count(); idx++)
    {
        if (widget->objectName().compare(
                layout->itemAt(idx)->widget()->objectName(),
                Qt::CaseInsensitive) < 0)
            break;
    }
    layout->insertWidget(idx, widget);
}

void StatusNotifier::itemAdded(const QString & serviceAndPath)
{
    int slash = serviceAndPath.indexOf('/');
    QString serv = serviceAndPath.left(slash);
    QString path = serviceAndPath.mid(slash);
    auto icon = new StatusNotifierIcon(serv, path, this);
    mServices.insert(serviceAndPath, icon);

    icon->hide();
    icon->getPropertyAsync("Title", [this, icon](const QVariant & value) {
        icon->setObjectName(qdbus_cast<QString>(value));
        insertSorted(&mLayout, icon);
        icon->show();
    });
}

void StatusNotifier::itemRemoved(const QString & serviceAndPath)
{
    auto icon = mServices.take(serviceAndPath);
    if (icon)
        icon->deleteLater();
}
