/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2010-2011 Razor team
 * Authors:
 *   Alexander Sokoloff <sokoloff.a@gmail.com>
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


#ifndef LXQTPANEL_H
#define LXQTPANEL_H

#include <QFrame>
#include <QString>
#include <QTimer>
#include <QPropertyAnimation>
#include <QPointer>
#include <LXQt/Settings>
#include "ilxqtpanel.h"
#include "lxqtpanelglobals.h"

class QHBoxLayout;
class QMenu;
class Plugin;

namespace LXQt {
class Settings;
}

/*! \brief The LXQtPanel class provides a single lxqt-panel. All LXQtPanel
 * instances should be created and handled by LXQtPanelApplication. In turn,
 * all Plugins should be created and handled by LXQtPanels.
 *
 * LXQtPanel is just the panel, it does not incorporate any functionality.
 * Each function of the panel is implemented by Plugins, even the mainmenu
 * (plugin-mainmenu) and the taskbar (plugin-taskbar). So the LXQtPanel is
 * just the container for several Plugins while the different Plugins
 * incorporate the functions of the panel. Without the Plugins, the panel
 * is quite useless because it is just a box occupying space on the screen.
 *
 * LXQtPanel itself is a window (QFrame/QWidget) and this class is mainly
 * responsible for handling the size and position of this window on the
 * screen(s) as well as the different settings. The handling of the plugins
 * is outsourced in PanelPluginsModel and LXQtPanelLayout. PanelPluginsModel
 * is responsible for loading/creating and handling the plugins.
 * LXQtPanelLayout is inherited from QLayout and set as layout to the
 * background of LXQtPanel, so LXQtPanelLayout is responsible for the
 * layout of all the Plugins.
 *
 * \sa LXQtPanelApplication, Plugin, PanelPluginsModel, LXQtPanelLayout.
 */
class LXQT_PANEL_API LXQtPanel : public QFrame, public ILXQtPanel
{
    Q_OBJECT

public:
    /**
     * @brief Creates and initializes the LXQtPanel. Performs the following
     * steps:
     * 1. Sets Qt window title, flags, attributes.
     * 2. Creates the panel layout.
     * 3. Prepares the timers.
     * 4. Connects signals and slots.
     * 5. Reads the settings for this panel.
     * 6. Optionally moves the panel to a valid screen (position-dependent).
     * 7. Loads the Plugins.
     * 8. Shows the panel, even if it is hidable (but then, starts the timer).
     * @param configGroup The name of the panel which is used as identifier
     * in the config file.
     * @param settings The settings instance of this lxqt panel application.
     * @param parent Parent QWidget, can be omitted.
     */
    LXQtPanel(QWidget *parent = 0);
    virtual ~LXQtPanel();

    // ILXQtPanel overrides ........
    QRect globalGeometry() const override;
    QRect calculatePopupWindowPos(QPoint const & absolutePos, QSize const & windowSize) const override;
    QRect calculatePopupWindowPos(const ILXQtPanelPlugin *plugin, const QSize &windowSize) const override;
    // ........ end of ILXQtPanel overrides

    /**
     * @brief Searches for a Plugin in the Plugins-list of this panel. Takes
     * an ILXQtPanelPlugin as parameter and returns the corresponding Plugin.
     * @param iPlugin ILXQtPanelPlugin that we are looking for.
     * @return The corresponding Plugin if it is loaded in this panel, nullptr
     * otherwise.
     */
    Plugin *findPlugin(const ILXQtPanelPlugin *iPlugin) const;

public slots:
    /**
     * @brief Shows the QWidget and makes it visible on all desktops. This
     * method is NOT related to showPanel(), hidePanel() and hidePanelWork()
     * which handle the LXQt hiding by resizing the panel.
     */
    void show();

signals:
    /**
     * @brief This signal gets emitted whenever this panel receives a
     * QEvent::LayoutRequest, i.e. "Widget layout needs to be redone.".
     * The PanelPluginsModel will connect this signal to the individual
     * plugins so they can realign, too.
     */
    void realigned();

protected:
    /**
     * @brief Overrides QObject::event(QEvent * e). Some functions of
     * the panel will be triggered by these events, e.g. showing/hiding
     * the panel or showing the context menu.
     * @param event The event that was received.
     * @return "QObject::event(QEvent *e) should return true if the event e
     * was recognized and processed." This is done by passing the event to
     * QFrame::event(QEvent *e) at the end.
     */
    bool event(QEvent *event) override;
    /**
     * @brief Overrides QWidget::showEvent(QShowEvent * event). This
     * method is called when a widget (in this case: the LXQtPanel) is
     * shown. The call could happen before and after the widget is shown.
     * This method is just overridden to get notified when the LXQtPanel
     * will be shown. Then, LXQtPanel will call realign().
     * @param event The QShowEvent sent by Qt.
     */
    void showEvent(QShowEvent *event) override;

private slots:
    /**
     * @brief Recalculates the geometry of the panel and reserves the
     * window manager strut, i.e. it calls setPanelGeometry() and
     * updateWmStrut().
     * Two signals will be connected to this slot:
     * 1. QDesktopWidget::workAreaResized(int screen) which will be emitted
     * when the work area available (on screen) changes.
     * 2. LXQt::Application::themeChanged(), i.e. when the user changes
     * the theme.
     */
    void realign();

private:
    QPointer<QScreen> mScreen;
    /**
     * @brief The LXQtPanelLayout of this panel. All the Plugins will be added
     * to the UI via this layout.
     */
    QHBoxLayout* mLayout;
    /**
     * @brief The background widget for the panel. This background widget will
     * have the background color or the background image if any of these is
     * set. This background widget will have the LXQtPanelLayout mLayout which
     * will in turn contain all the Plugins.
     */
    QFrame *LXQtPanelWidget;
    /**
     * @brief Pointer to the PanelPluginsModel which will store all the Plugins
     * that are loaded.
     */
    QList<Plugin *> mPlugins;

    /**
     * @brief Update the window manager struts _NET_WM_PARTIAL_STRUT and
     * _NET_WM_STRUT for this widget. "The purpose of struts is to reserve
     * space at the borders of the desktop. This is very useful for a
     * docking area, a taskbar or a panel, for instance. The Window Manager
     * should take this reserved area into account when constraining window
     * positions - maximized windows, for example, should not cover that
     * area."
     * \sa http://standards.freedesktop.org/wm-spec/wm-spec-latest.html#NETWMSTRUT
     */
    void updateWmStrut();

    /**
     * @brief Loads the plugins, i.e. creates a new PanelPluginsModel.
     * Connects the signals and slots and adds all the plugins to the
     * layout.
     */
    void loadPlugins();

    /**
     * @brief Calculates and sets the geometry (i.e. the position and the size
     * on the screen) of the panel. Considers alignment, position, if the panel
     * is hidden and if its geometry should be set with animation.
     * \param animate flag if showing/hiding the panel should be animated.
     */
    void setPanelGeometry();
    /**
     * @brief Sets the contents margins of the panel according to its position
     * and hiddenness. All margins are zero for visible panels.
     */
    void setMargins();
};


#endif // LXQTPANEL_H
