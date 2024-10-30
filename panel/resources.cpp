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
#include <QRegularExpression>

#undef signals
#include <gio/gdesktopappinfo.h>
#include <gio/gio.h>

AppInfo::AppInfo(GDesktopAppInfo * info)
    : mInfo((GDesktopAppInfo *)g_object_ref(info), g_object_unref)
{
}

QStringList AppInfo::categories() const
{
    if (!g_app_info_should_show((GAppInfo *)mInfo.get()))
        return QStringList();

    return QString(g_desktop_app_info_get_categories(mInfo.get()))
        .split(';', Qt::SkipEmptyParts);
}

QIcon AppInfo::getIcon() const
{
    auto gicon = g_app_info_get_icon((GAppInfo *)mInfo.get());
    if (!gicon)
        return QIcon();

    CharPtr name(g_icon_to_string(gicon), g_free);
    return name ? Resources::getIcon(QString(name)) : QIcon();
}

QString AppInfo::getExecutable() const
{
    return g_app_info_get_executable((GAppInfo *)mInfo.get());
}

QString AppInfo::getStartupWMClass() const
{
    return g_desktop_app_info_get_startup_wm_class(mInfo.get());
}

QAction * AppInfo::getAction()
{
    if (mAction)
        return mAction.get();

    auto info = mInfo.get();
    auto action =
        new QAction(getIcon(), g_app_info_get_display_name((GAppInfo *)info));

    QObject::connect(action, &QAction::triggered, [info]() {
        // Unset QT_WAYLAND_SHELL_INTEGRATION or else all launched
        // Qt applications will use layer-shell, wanted or not
        auto context = g_app_launch_context_new();
        g_app_launch_context_unsetenv(context, "QT_WAYLAND_SHELL_INTEGRATION");
        if (!g_desktop_app_info_launch_uris_as_manager(
                info, nullptr, context, G_SPAWN_SEARCH_PATH, restore_signals,
                nullptr, nullptr, nullptr, nullptr))
            qWarning() << "Failed to launch"
                       << g_app_info_get_id((GAppInfo *)info);
        g_object_unref(context);
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
            auto path = QString("%1/%2.%3").arg(dir, name, ext);
            if (g_file_test(path.toUtf8(), G_FILE_TEST_EXISTS))
                return QIcon(path);
        }
    }

    qWarning() << "Cannot load icon" << name;
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
        if (G_IS_DESKTOP_APP_INFO(app))
            apps.emplace(g_app_info_get_id(app), (GDesktopAppInfo *)app);
    }

    return apps;
}

// Create mapping of short application name to full .desktop file name
// Example: thunderbird -> org.mozilla.Thunderbird.desktop
Resources::AppNameMap Resources::makeAppNameMap(AppInfoMap & appInfos)
{
    static QRegularExpression desktopExtRegEx("\\.desktop$");
    static QRegularExpression beforeDotRegEx(".*\\.");
    static QRegularExpression beforeSlashRegEx(".*\\/");

    auto nameMap = AppNameMap();
    for (auto & pair : appInfos)
    {
        QString name = pair.first;
        name.remove(desktopExtRegEx);
        name.remove(beforeDotRegEx);
        nameMap.emplace(name.toLower(), pair.first);

        // Also map by executable name (if different than app name)
        // and StartupWMClass key. Maybe it's redundant to check both?
        // Either one gives (for example): gimp-2.10 -> gimp.desktop.
        // But "VirtualBox Manager" matches only StartupWMClass.
        QString execName = pair.second.getExecutable();
        execName.remove(beforeSlashRegEx);
        if (!execName.isEmpty())
            nameMap.emplace(execName.toLower(), pair.first);

        QString wmClass = pair.second.getStartupWMClass();
        if (!wmClass.isEmpty())
            nameMap.emplace(wmClass.toLower(), pair.first);
    }

    return nameMap;
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

QIcon Resources::getAppIcon(const QString & appName)
{
    // try exact match of appName + ".desktop" first
    auto iter = mAppInfos.find(appName + ".desktop");
    if (iter != mAppInfos.end())
        return iter->second.getIcon();

    // try known short application names
    auto nameIter = mAppNameMap.find(appName.toLower());
    if (nameIter != mAppNameMap.end())
    {
        iter = mAppInfos.find(nameIter->second);
        if (iter != mAppInfos.end())
            return iter->second.getIcon();
    }

    qWarning() << "No icon available for" << appName;
    return QIcon();
}

// note: appID includes ".desktop" suffix
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
