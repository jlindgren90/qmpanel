/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2015 LXQt team
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

#include "panelpluginsmodel.h"
#include "plugin.h"
#include "ilxqtpanelplugin.h"
#include "lxqtpanel.h"
#include "lxqtpanelapplication.h"
#include <QPointer>
#include <XdgIcon>
#include <LXQt/Settings>

#include <QDebug>

PanelPluginsModel::PanelPluginsModel(LXQtPanel * panel,
                                     QString const & namesKey,
                                     QStringList const & desktopDirs,
                                     QObject * parent/* = nullptr*/)
    :
    mNamesKey(namesKey),
    mPanel(panel)
{
    loadPlugins(desktopDirs);
}

PanelPluginsModel::~PanelPluginsModel()
{
    qDeleteAll(plugins());
}

QList<Plugin *> PanelPluginsModel::plugins() const
{
    QList<Plugin *> plugins;
    for (auto const & p : mPlugins)
        if (!p.second.isNull())
            plugins.append(p.second.data());
    return plugins;
}

void PanelPluginsModel::loadPlugins(QStringList const & desktopDirs)
{
    QStringList plugin_names = mPanel->settings()->value(mNamesKey).toStringList();

    for (auto const & name : plugin_names)
    {
        pluginslist_t::iterator i = mPlugins.insert(mPlugins.end(), {name, nullptr});
        QString type = mPanel->settings()->value(name + "/type").toString();
        if (type.isEmpty())
        {
            qWarning() << QString("Section \"%1\" not found in %2.").arg(name, mPanel->settings()->fileName());
            continue;
        }

        i->second = loadPlugin(type);
    }
}

QPointer<Plugin> PanelPluginsModel::loadPlugin(QString const & type)
{
    std::unique_ptr<Plugin> plugin(new Plugin(type, mPanel));
    if (plugin->isLoaded())
    {
        QObject::connect(mPanel, &LXQtPanel::realigned, plugin.get(), &Plugin::realign);
        return plugin.release();
    }

    return nullptr;
}
