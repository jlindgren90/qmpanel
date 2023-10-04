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

#include "statusnotifiericon.h"
#include "../../dbusmenu/dbusmenuimporter.h"

#include <QMenu>
#include <QMouseEvent>
#include <QStyle>
#include <QtEndian>

StatusNotifierIcon::StatusNotifierIcon(QString service, QString objectPath,
                                       QWidget * parent)
    : QLabel(parent), mSni(service, objectPath, QDBusConnection::sessionBus())
{
    connect(&mSni, &org::kde::StatusNotifierItem::NewIcon, this,
            &StatusNotifierIcon::newIcon);
    connect(&mSni, &org::kde::StatusNotifierItem::NewToolTip, this,
            &StatusNotifierIcon::newToolTip);

    getPropertyAsync("Menu", [this](const QVariant & value) {
        auto path = qdbus_cast<QDBusObjectPath>(value);
        if (!path.path().isEmpty())
        {
            auto importer =
                new DBusMenuImporter(mSni.service(), path.path(), this);
            mMenu = importer->menu();
            connect(importer, &DBusMenuImporter::menuUpdated, this,
                    &StatusNotifierIcon::menuUpdated);
        }
    });

    newIcon();
    newToolTip();
}

void StatusNotifierIcon::getPropertyAsync(
    QString const & name, std::function<void(const QVariant &)> finished)
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

void StatusNotifierIcon::menuUpdated()
{
    // always recreate "Activate" action since
    // DBusMenuImporter may call deleteLater() on it
    if (mActivate)
        mActivate->deleteLater();

    if (!mMenu || mMenu->isEmpty())
        return;

    mActivate = new QAction("Activate", this);
    auto font = mActivate->font();
    font.setBold(true);
    mActivate->setFont(font);

    connect(mActivate, &QAction::triggered, [this]() {
        auto pos = mapToGlobal(QPoint()); // left top corner
        mSni.Activate(pos.x(), pos.y());
    });

    mMenu->addAction(mActivate);
}

static QIcon iconFromPixmapList(IconPixmapList & pixmaps)
{
    QIcon icon;
    for (auto & pixmap : pixmaps)
    {
        int w = pixmap.width;
        int h = pixmap.height;

        if (pixmap.bytes.length() != w * h * 4)
            continue;

        void * data = pixmap.bytes.data();
        qFromBigEndian<qint32>(data, w * h, data);
        icon.addPixmap(QPixmap::fromImage(
            QImage((uchar *)data, w, h, QImage::Format_ARGB32)));
    }

    return icon;
}

void StatusNotifierIcon::newIcon()
{
    getPropertyAsync("IconName", [this](const QVariant & value) {
        auto iconName = qdbus_cast<QString>(value);
        if (!iconName.isEmpty())
        {
            setPixmap(QIcon::fromTheme(iconName).pixmap(
                style()->pixelMetric(QStyle::PM_ButtonIconSize)));
        }
        else
        {
            getPropertyAsync("IconPixmap", [this](const QVariant & value) {
                auto pixmaps = qdbus_cast<IconPixmapList>(value);
                auto icon = iconFromPixmapList(pixmaps);
                if (!icon.isNull())
                {
                    setPixmap(icon.pixmap(
                        style()->pixelMetric(QStyle::PM_ButtonIconSize)));
                }
            });
        }
    });
}

void StatusNotifierIcon::newToolTip()
{
    getPropertyAsync("ToolTip", [this](const QVariant & value) {
        auto tooltip = qdbus_cast<ToolTip>(value);
        setToolTip(tooltip.title);
    });
}

void StatusNotifierIcon::mousePressEvent(QMouseEvent * event)
{
    auto pos = mapToGlobal(QPoint()); // left top corner

    if (event->button() == Qt::LeftButton)
    {
        if (mMenu && !mMenu->isEmpty())
        {
            pos.ry() -= mMenu->sizeHint().height();
            mMenu->popup(pos);
        }
        else
            mSni.Activate(pos.x(), pos.y());
    }
    else if (event->button() == Qt::MiddleButton)
        mSni.SecondaryActivate(pos.x(), pos.y());
    else if (Qt::RightButton == event->button())
        mSni.ContextMenu(pos.x(), pos.y());
}
