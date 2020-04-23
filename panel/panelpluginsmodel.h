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

#ifndef PANELPLUGINSMODEL_H
#define PANELPLUGINSMODEL_H

#include <QAbstractListModel>
#include <memory>

namespace LXQt
{
    class PluginInfo;
    struct PluginData;
}

class LXQtPanel;
class Plugin;

/*!
 * \brief The PanelPluginsModel class implements the Model part of the
 * Qt Model/View architecture for the Plugins, i.e. it is the interface
 * to access the Plugin data associated with this Panel. The
 * PanelPluginsModel takes care for read-access as well as changes
 * like adding, removing or moving Plugins.
 */
class PanelPluginsModel
{
public:
    PanelPluginsModel(LXQtPanel * panel,
                      QString const & namesKey,
                      QStringList const & desktopDirs,
                      QObject * parent = nullptr);
    ~PanelPluginsModel();

    /*!
     * \brief plugins returns a list of Plugins in this panel.
     */
    QList<Plugin *> plugins() const;

private:
    /*!
     * \brief pluginslist_t is the data type used for mPlugins which stores
     * all the Plugins.
     *
     * \sa mPlugins
     */
    typedef QList<QPair <QString/*name*/, QPointer<Plugin> > > pluginslist_t;

private:
    /*!
     * \brief loadPlugins Loads all the Plugins.
     * \param desktopDirs These directories are scanned for corresponding
     * .desktop-files which are necessary to load the plugins.
     */
    void loadPlugins(QStringList const & desktopDirs);
    /*!
     * \brief loadPlugin Loads a Plugin and connects signals and slots.
     * \param desktopFile The desktop file that specifies how to load the
     * Plugin.
     * \param settingsGroup QString which specifies the settings group. This
     * will only be redirected to the Plugin so that it knows how to read
     * its settings.
     * \return A QPointer to the Plugin that was loaded.
     */
    QPointer<Plugin> loadPlugin(QString const & type);

    /*!
     * \brief mNamesKey The key to the settings-entry that stores the
     * names of the Plugins in a panel. Set upon creation, passed as
     * a parameter by the panel.
     */
    const QString mNamesKey;
    /*!
     * \brief mPlugins Stores all the Plugins.
     *
     * mPlugins is a QList of elements while each element corresponds to a
     * single Plugin. Each element is a QPair of a QString and a QPointer
     * while the QPointer points to a Plugin.
     *
     * To access the elements, you can use indexing or an iterator on the
     * list. For each element p, p.first is the name of the Plugin as it
     * is used in the configuration files, p.second.data() is the Plugin.
     *
     * \sa pluginslist_t
     */
    pluginslist_t mPlugins;
    /*!
     * \brief mPanel Stores a reference to the LXQtPanel.
     */
    LXQtPanel * mPanel;
};

Q_DECLARE_METATYPE(Plugin const *)

#endif // PANELPLUGINSMODEL_H
