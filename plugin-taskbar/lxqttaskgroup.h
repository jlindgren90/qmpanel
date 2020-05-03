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
 *   Maciej Płaza <plaza.maciej@gmail.com>
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

#ifndef LXQTTASKGROUP_H
#define LXQTTASKGROUP_H

#include "../panel/plugin.h"
#include "lxqttaskbar.h"
#include "lxqttaskbutton.h"
#include <KF5/KWindowSystem/kwindowsystem.h>

class QVBoxLayout;
class Plugin;

class LXQtGroupPopup;
class LXQtMasterPopup;

class LXQtTaskGroup: public LXQtTaskButton
{
    Q_OBJECT

public:
    LXQtTaskGroup(const QString & groupName, WId window, LXQtTaskBar * parent);

    QString groupName() const { return mGroupName; }

    int buttonsCount() const;
    int visibleButtonsCount() const;

    LXQtTaskButton * addWindow(WId id);
    LXQtTaskButton * checkedButton() const;

    bool onWindowChanged(WId window, NET::Properties prop, NET::Properties2 prop2);

public slots:
    void onWindowRemoved(WId window);

protected:
    QMimeData * mimeData();

private slots:
    void onActiveWindowChanged(WId window);
    void onDesktopChanged(int number);

    void closeGroup();
    void refreshVisibility();

signals:
    void groupBecomeEmpty(QString name);
    void visibilityChanged(bool visible);

private:
    QString mGroupName;
    LXQtTaskButtonHash mButtonHash;

    void regroup();
};

#endif // LXQTTASKGROUP_H
