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

#include "settings.h"
#include "utils.h"

#include <glib.h>

Settings loadSettings()
{
    AutoPtr<GKeyFile> kf(g_key_file_new(), g_key_file_unref);
    auto path = QString(g_get_user_config_dir()) + "/qmpanel.ini";

    g_key_file_load_from_file(kf.get(), path.toUtf8(), G_KEY_FILE_NONE,
                              nullptr);

    auto getSetting = [&](const char * key) {
        return CharPtr(g_key_file_get_value(kf.get(), "Settings", key, nullptr),
                       g_free);
    };

    QString menuIcon = getSetting("MenuIcon");
    QString pinnedMenuApps = getSetting("PinnedMenuApps");
    QString quickLaunchApps = getSetting("QuickLaunchApps");

    return {menuIcon.isEmpty() ? "start-here" : menuIcon,
            pinnedMenuApps.split(';', QString::SkipEmptyParts),
            quickLaunchApps.split(';', QString::SkipEmptyParts)};
}
