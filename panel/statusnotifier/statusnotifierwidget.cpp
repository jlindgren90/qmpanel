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

#include "statusnotifierwidget.h"
#include "statusnotifierbutton.h"
#include "statusnotifierproxy.h"

#include <QBoxLayout>

StatusNotifierWidget::StatusNotifierWidget(QWidget * parent) : QWidget(parent)
{
    auto hlayout = new QHBoxLayout(this);
    hlayout->setContentsMargins(0, 0, 0, 0);
    hlayout->setSpacing(0);
    setLayout(hlayout);

    StatusNotifierProxy & proxy =
        StatusNotifierProxy::registerLifetimeUsage(this);
    connect(&proxy, &StatusNotifierProxy::StatusNotifierItemRegistered, this,
            &StatusNotifierWidget::itemAdded);
    connect(&proxy, &StatusNotifierProxy::StatusNotifierItemUnregistered, this,
            &StatusNotifierWidget::itemRemoved);
    for (const auto & service : proxy.RegisteredStatusNotifierItems())
        itemAdded(service);
}

void StatusNotifierWidget::itemAdded(const QString & serviceAndPath)
{
    int slash = serviceAndPath.indexOf(QLatin1Char('/'));
    QString serv = serviceAndPath.left(slash);
    QString path = serviceAndPath.mid(slash);
    StatusNotifierButton * button = new StatusNotifierButton(serv, path, this);

    mServices.insert(serviceAndPath, button);
    layout()->addWidget(button);
    button->show();
}

void StatusNotifierWidget::itemRemoved(const QString & serviceAndPath)
{
    StatusNotifierButton * button = mServices.value(serviceAndPath, nullptr);
    if (button)
    {
        button->deleteLater();
        mServices.remove(serviceAndPath);
    }
}
