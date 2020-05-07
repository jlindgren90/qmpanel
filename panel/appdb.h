/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * qmpanel - a minimal Qt-based desktop panel
 *
 * Copyright: 2020 John Lindgren
 * Authors:
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

#ifndef APPDB_H
#define APPDB_H

#include <QList>
#include <unordered_map>
#include <unordered_set>

#include "utils.h"

class QAction;
class QIcon;
class QObject;

typedef struct _GAppInfo GAppInfo;

#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
template<>
struct std::hash<QString>
{
    std::size_t operator()(const QString & v) const noexcept
    {
        return qHash(v);
    }
};
#endif

class AppDB
{
public:
    AppDB();

    static QIcon getIcon(const QString & name);
    static QIcon getIcon(GAppInfo * info);

    QAction * createAction(const QString & appID, QObject * parent) const;

    QList<QAction *> createCategory(const QString & category,
                                    std::unordered_set<QString> & added,
                                    QObject * parent) const;

private:
    std::unordered_map<QString, AutoPtrV<GAppInfo>> mAppInfos;
};

#endif
