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

#include "resources.h"

#include <QAction>
#include <QDebug>
#include <QFileInfo>
#include <signal.h>

#undef signals
#include <gio/gdesktopappinfo.h>
#include <gio/gio.h>

AppInfo::AppInfo(GDesktopAppInfo * info)
    : mInfo((GDesktopAppInfo *)g_object_ref(info), g_object_unref)
{
}

QStringList AppInfo::categories() const
{
    return QString(g_desktop_app_info_get_categories(mInfo.get()))
        .split(';', Qt::SkipEmptyParts);
}

QAction * AppInfo::getAction()
{
    if (mAction)
        return mAction.get();

    auto info = mInfo.get();
    auto getIcon = [info]() {
        auto gicon = g_app_info_get_icon((GAppInfo *)info);
        if (!gicon)
            return QIcon();

        CharPtr name(g_icon_to_string(gicon), g_free);
        return name ? Resources::getIcon(QString(name)) : QIcon();
    };

    auto action =
        new QAction(getIcon(), g_app_info_get_display_name((GAppInfo *)info));

    QObject::connect(action, &QAction::triggered, [info]() {
        if (!g_desktop_app_info_launch_uris_as_manager(
                info, nullptr, nullptr, G_SPAWN_SEARCH_PATH, restore_signals,
                nullptr, nullptr, nullptr, nullptr))
            qWarning() << "Failed to launch"
                       << g_app_info_get_id((GAppInfo *)info);
    });

    mAction.reset(action);
    return action;
}

QIcon Resources::getIcon(const QString & name)
{
    if (g_path_is_absolute(name.toUtf8()))
        return QIcon(name);

    auto icon = QIcon::fromTheme(name);
    if (!icon.isNull())
        return icon;

    for (auto dir : {"/usr/share/icons", "/usr/share/pixmaps"})
    {
        for (auto ext : {"svg", "png", "xpm"})
        {
            auto path = QString("%1/%2.%3").arg(dir).arg(name).arg(ext);
            if (g_file_test(path.toUtf8(), G_FILE_TEST_EXISTS))
                return QIcon(path);
        }
    }

    return QIcon();
}

Resources::AppInfoMap Resources::loadAppInfos()
{
    AppInfoMap apps;

    AutoPtr<GList> list(g_app_info_get_all(), [](GList * list) {
        g_list_free_full(list, g_object_unref);
    });

    for (auto node = list.get(); node; node = node->next)
    {
        auto app = (GAppInfo *)node->data;
        if (G_IS_DESKTOP_APP_INFO(app) && g_app_info_should_show(app))
            apps.emplace(g_app_info_get_id(app), (GDesktopAppInfo *)app);
    }

    return apps;
}

Resources::Settings Resources::loadSettings()
{
    AutoPtr<GKeyFile> kf(g_key_file_new(), g_key_file_unref);
    auto path = QString(g_get_user_config_dir()) + "/qmpanel.ini";

    g_key_file_load_from_file(kf.get(), path.toUtf8(), G_KEY_FILE_NONE,
                              nullptr);

    auto getSetting = [&](const char * key) {
        return QString(CharPtr(
            g_key_file_get_value(kf.get(), "Settings", key, nullptr), g_free));
    };

    auto menuIcon = getSetting("MenuIcon");
    auto pinnedMenuApps = getSetting("PinnedMenuApps");
    auto quickLaunchApps = getSetting("QuickLaunchApps");
    auto launchCmds = getSetting("LaunchCmds");

    return {menuIcon.isEmpty() ? "start-here" : menuIcon,
            pinnedMenuApps.split(';', Qt::SkipEmptyParts),
            quickLaunchApps.split(';', Qt::SkipEmptyParts),
            launchCmds.split(';', Qt::SkipEmptyParts)};
}

QAction * Resources::getAction(const QString & appID)
{
    auto iter = mAppInfos.find(appID);
    if (iter != mAppInfos.end())
        return iter->second.getAction();

    qWarning() << "Unknown application" << appID;
    return nullptr;
}

QList<QAction *> Resources::getCategory(const QString & category,
                                        std::unordered_set<QString> & added)
{
    QList<QAction *> actions;

    for (auto & pair : mAppInfos)
    {
        // only add if not already in another category
        if (pair.second.categories().contains(category, Qt::CaseInsensitive) &&
            added.insert(pair.first).second)
        {
            actions.append(pair.second.getAction());
        }
    }

    std::sort(actions.begin(), actions.end(), [](QAction * a, QAction * b) {
        return (a->text().compare(b->text(), Qt::CaseInsensitive) < 0);
    });

    return actions;
}
