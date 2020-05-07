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

#include "appdb.h"

#include <QAction>
#include <QDebug>
#include <QFileInfo>

#undef signals
#include <gio/gdesktopappinfo.h>
#include <gio/gio.h>

static QIcon getIcon(GAppInfo * info)
{
    auto gicon = g_app_info_get_icon(info);
    if (!gicon)
        return QIcon();

    CharPtr name(g_icon_to_string(gicon), g_free);
    if (!name)
        return QIcon();

    if (g_path_is_absolute(name))
        return QIcon(name);

    auto icon = QIcon::fromTheme(name);
    if (!icon.isNull())
        return icon;

    for (auto ext : {".svg", ".png", ".xpm"})
    {
        CharPtr path(
            g_strconcat("/usr/share/pixmaps/", name.get(), ext, nullptr),
            g_free);

        if (g_file_test(path, G_FILE_TEST_EXISTS))
            return QIcon(path);
    }

    return QIcon();
}

class AppAction : public QAction
{
public:
    AppAction(GAppInfo * info, QObject * parent)
        : QAction(getIcon(info), g_app_info_get_display_name(info), parent),
          mInfo((GAppInfo *)g_object_ref(info), g_object_unref)
    {
        connect(this, &QAction::triggered, [info]() {
            if (!g_app_info_launch(info, nullptr, nullptr, nullptr))
                qWarning() << "Failed to launch" << g_app_info_get_id(info);
        });
    }

private:
    AutoPtrV<GAppInfo> mInfo;
};

AppDB::AppDB()
{
    AutoPtr<GList> list(g_app_info_get_all(), [](GList * list) {
        g_list_free_full(list, g_object_unref);
    });

    for (auto node = list.get(); node; node = node->next)
    {
        auto app = (GAppInfo *)node->data;
        if (!g_app_info_should_show(app))
            continue;

        mAppInfos.emplace(std::piecewise_construct,
                          std::forward_as_tuple(g_app_info_get_id(app)),
                          std::forward_as_tuple((GAppInfo *)g_object_ref(app),
                                                g_object_unref));
    }
}

QAction * AppDB::createAction(const char * appID, QObject * parent) const
{
    auto iter = mAppInfos.find(appID);
    if (iter == mAppInfos.end())
    {
        qWarning() << "Failed to create action for" << appID;
        return nullptr;
    }

    return new AppAction(iter->second.get(), parent);
}

QList<QAction *> AppDB::createCategory(const char * category,
                                       QObject * parent) const
{
    QList<QAction *> actions;

    for (auto & pair : mAppInfos)
    {
        auto info = pair.second.get();
        if (!G_IS_DESKTOP_APP_INFO(info))
            continue;

        auto categories =
            QString(g_desktop_app_info_get_categories((GDesktopAppInfo *)info))
                .split(';', QString::SkipEmptyParts);

        if (categories.contains(category, Qt::CaseInsensitive))
            actions.append(new AppAction(info, parent));
    }

    std::sort(actions.begin(), actions.end(), [](QAction * a, QAction * b) {
        return (a->text().compare(b->text(), Qt::CaseInsensitive) < 0);
    });

    return actions;
}
