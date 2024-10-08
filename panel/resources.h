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

#ifndef RESOURCES_H
#define RESOURCES_H

#include "utils.h"

#include <QAction>
#include <QStringList>
#include <unordered_map>
#include <unordered_set>

void restore_signals(void *); // from main.cpp

typedef struct _GDesktopAppInfo GDesktopAppInfo;

class AppInfo
{
public:
    explicit AppInfo(GDesktopAppInfo * info);

    QStringList categories() const;
    QIcon getIcon() const;
    QString getExecutable() const;
    QString getStartupWMClass() const;
    QAction * getAction();

private:
    AutoPtrV<GDesktopAppInfo> mInfo;
    std::unique_ptr<QAction> mAction;
};

class Resources
{
public:
    struct Settings
    {
        QString menuIcon;
        QStringList pinnedMenuApps;
        QStringList quickLaunchApps;
        QStringList launchCmds;
    };

    static QIcon getIcon(const QString & name);

    const Settings & settings() const { return mSettings; }

    QIcon getAppIcon(const QString & appName);
    QAction * getAction(const QString & appID);
    QList<QAction *> getCategory(const QString & category,
                                 std::unordered_set<QString> & added);

private:
    using AppInfoMap = std::unordered_map<QString, AppInfo>;
    using AppNameMap = std::unordered_map<QString, QString>;

    static AppInfoMap loadAppInfos();
    static AppNameMap makeAppNameMap(AppInfoMap & appInfos);
    static Settings loadSettings();

    AppInfoMap mAppInfos = loadAppInfos();
    AppNameMap mAppNameMap = makeAppNameMap(mAppInfos);
    Settings mSettings = loadSettings();
};

#endif
