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

#include <QHBoxLayout>
#include <QPointer>
#include <QWidget>

class Plugin;

/*! \brief The LXQtPanel class provides a single lxqt-panel.
 *
 * LXQtPanel is just the panel, it does not incorporate any functionality.
 * Each function of the panel is implemented by Plugins, even the mainmenu
 * (plugin-mainmenu) and the taskbar (plugin-taskbar). So the LXQtPanel is
 * just the container for several Plugins while the different Plugins
 * incorporate the functions of the panel. Without the Plugins, the panel
 * is quite useless because it is just a box occupying space on the screen.
 *
 * LXQtPanel itself is a window (QWidget) and this class is mainly
 * responsible for handling the size and position of this window on the
 * screen.
 */
class LXQtPanel : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief Creates and initializes the LXQtPanel.
     */
    LXQtPanel(QWidget * parent = 0);

    QRect calculatePopupWindowPos(QPoint const & absolutePos,
                                  QSize const & windowSize) const;
    QRect calculatePopupWindowPos(QWidget * widget,
                                  const QSize & windowSize) const;

    /**
     * @brief Shows the QWidget and makes it visible on all desktops.
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
     * the panel will be triggered by these events.
     * @param event The event that was received.
     * @return "QObject::event(QEvent *e) should return true if the event e
     * was recognized and processed." This is done by passing the event to
     * QWidget::event(QEvent *e) at the end.
     */
    bool event(QEvent * event) override;
    /**
     * @brief Overrides QWidget::showEvent(QShowEvent * event). This
     * method is called when a widget (in this case: the LXQtPanel) is
     * shown. The call could happen before and after the widget is shown.
     * This method is just overridden to get notified when the LXQtPanel
     * will be shown. Then, LXQtPanel will call realign().
     * @param event The QShowEvent sent by Qt.
     */
    void showEvent(QShowEvent * event) override;

private:
    /**
     * @brief Recalculates the geometry of the panel and reserves the
     * window manager strut, i.e. it calls setPanelGeometry() and
     * updateWmStrut().
     */
    void realign();

    QPointer<QScreen> mScreen;
    QHBoxLayout mLayout;

    /**
     * @brief Update the window manager struts _NET_WM_PARTIAL_STRUT and
     * _NET_WM_STRUT for this widget. "The purpose of struts is to reserve
     * space at the borders of the desktop. This is very useful for a
     * docking area, a taskbar or a panel, for instance. The Window Manager
     * should take this reserved area into account when constraining window
     * positions - maximized windows, for example, should not cover that
     * area."
     */
    void updateWmStrut();

    /**
     * @brief Loads the plugins and adds them to the layout.
     */
    void loadPlugins();

    /**
     * @brief Calculates and sets the geometry (i.e. the position and the size
     * on the screen) of the panel.
     */
    void setPanelGeometry();
};

#endif // LXQTPANEL_H
