/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
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
#include <string>
#include <unordered_map>

#include "utils.h"

class QAction;
class QObject;

typedef struct _GAppInfo GAppInfo;

class AppDB
{
public:
    AppDB();

    QAction * createAction(const char * appID, QObject * parent) const;
    QList<QAction *> createCategory(const char * category,
                                    QObject * parent) const;

private:
    std::unordered_map<std::string, AutoPtrV<GAppInfo>> mAppInfos;
};

#endif
