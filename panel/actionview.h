/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * qmpanel - a minimal Qt-based desktop panel
 *
 * Copyright: 2016 LXQt team
 *            2020 John Lindgren
 * Authors:
 *   Palo Kisa <palo.kisa@gmail.com>
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

#ifndef ACTION_VIEW_H
#define ACTION_VIEW_H

#include <QListView>

class QStandardItemModel;
class FilterProxyModel;

class ActionView : public QListView
{
public:
    ActionView(QWidget * parent = nullptr);

    void addActions(QList<QAction *> actions);
    void setSearchStr(const QString & str);
    void activateCurrent();

protected:
    QSize viewportSizeHint() const override;
    QSize minimumSizeHint() const override { return QSize(); }

private:
    void onActivated(QModelIndex const & index);

    QStandardItemModel * mModel;
    FilterProxyModel * mProxy;
};

#endif // ACTION_VIEW_H
