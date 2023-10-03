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

#ifndef STATUSNOTIFIERICON_H
#define STATUSNOTIFIERICON_H

#include <QLabel>
#include <QPointer>
#include <functional>

#include "statusnotifieriteminterface.h"

class QMenu;

class StatusNotifierIcon : public QLabel
{
public:
    StatusNotifierIcon(QString service, QString objectPath,
                       QWidget * parent = nullptr);

    void getPropertyAsync(QString const & name,
                          std::function<void(const QVariant &)> finished);

private:
    void menuUpdated();
    void newIcon();
    void newToolTip();

    org::kde::StatusNotifierItem mSni;
    QPointer<QMenu> mMenu;
    QPointer<QAction> mActivate;

protected:
    void mousePressEvent(QMouseEvent * event);
};

#endif // STATUSNOTIFIERICON_H
