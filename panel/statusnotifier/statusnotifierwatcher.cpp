/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * qmpanel - a minimal Qt-based desktop panel
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

#include "statusnotifierwatcher.h"
#include <QDBusConnectionInterface>
#include <QDebug>

StatusNotifierWatcher::StatusNotifierWatcher(QObject * parent) : QObject(parent)
{
    qRegisterMetaType<IconPixmap>("IconPixmap");
    qDBusRegisterMetaType<IconPixmap>();
    qRegisterMetaType<IconPixmapList>("IconPixmapList");
    qDBusRegisterMetaType<IconPixmapList>();
    qRegisterMetaType<ToolTip>("ToolTip");
    qDBusRegisterMetaType<ToolTip>();

    auto dbus = QDBusConnection::sessionBus();
    auto reply = dbus.interface()->registerService(
        "org.kde.StatusNotifierWatcher",
        QDBusConnectionInterface::QueueService);

    switch (reply.value())
    {
    case QDBusConnectionInterface::ServiceNotRegistered:
        qWarning() << "StatusNotifier: unable to register service for "
                      "org.kde.StatusNotifierWatcher";
        break;
    case QDBusConnectionInterface::ServiceQueued:
        qWarning() << "StatusNotifier: registration of service "
                      "org.kde.StatusNotifierWatcher queued, we can become "
                      "primary after existing one deregisters";
        break;
    case QDBusConnectionInterface::ServiceRegistered:
        break;
    }

    if (!dbus.registerObject("/StatusNotifierWatcher", this,
                             QDBusConnection::ExportScriptableContents))
        qWarning() << QDBusConnection::sessionBus().lastError().message();

    mWatcher = new QDBusServiceWatcher(this);
    mWatcher->setConnection(dbus);
    mWatcher->setWatchMode(QDBusServiceWatcher::WatchForUnregistration);

    connect(mWatcher, &QDBusServiceWatcher::serviceUnregistered, this,
            &StatusNotifierWatcher::serviceUnregistered);
}

StatusNotifierWatcher::~StatusNotifierWatcher()
{
    QDBusConnection::sessionBus().unregisterService(
        "org.kde.StatusNotifierWatcher");
}

void StatusNotifierWatcher::RegisterStatusNotifierItem(
    const QString & serviceOrPath)
{
    QString service = serviceOrPath;
    QString path = "/StatusNotifierItem";

    // workaround for sni-qt
    if (service.startsWith(QLatin1Char('/')))
    {
        path = service;
        service = message().service();
    }

    QString notifierItemId = service + path;
    auto dbus = QDBusConnection::sessionBus();
    auto reply = dbus.interface()->isServiceRegistered(service);

    if (reply.value() && !mServices.contains(notifierItemId))
    {
        mServices << notifierItemId;
        mWatcher->addWatchedService(service);
        emit StatusNotifierItemRegistered(notifierItemId);
    }
}

void StatusNotifierWatcher::RegisterStatusNotifierHost(const QString & service)
{
    if (!mHosts.contains(service))
    {
        mHosts.append(service);
        mWatcher->addWatchedService(service);
    }
}

void StatusNotifierWatcher::serviceUnregistered(const QString & service)
{
    mWatcher->removeWatchedService(service);

    if (mHosts.contains(service))
    {
        mHosts.removeAll(service);
        return;
    }

    QString match = service + QLatin1Char('/');
    QStringList::Iterator it = mServices.begin();
    while (it != mServices.end())
    {
        if (it->startsWith(match))
        {
            QString name = *it;
            it = mServices.erase(it);
            emit StatusNotifierItemUnregistered(name);
        }
        else
            ++it;
    }
}
