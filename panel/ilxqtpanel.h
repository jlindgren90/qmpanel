/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2012 Razor team
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


#ifndef ILXQTPANEL_H
#define ILXQTPANEL_H
#include <QRect>
#include "lxqtpanelglobals.h"

class ILXQtPanelPlugin;
class QWidget;

/**
 **/
class LXQT_PANEL_API ILXQtPanel
{
public:
    /**
     * @brief Helper method that returns the global screen coordinates of the
     * panel, so you do not need to use QWidget::mapToGlobal() by yourself.
     * @return The QRect where the panel is located in global screen
     * coordinates.
     */
    virtual QRect globalGeometry() const = 0;

    /**
     * @brief Helper method for calculating the global screen position of a
     * popup window with size windowSize.
     * @param absolutePos Contains the global screen coordinates where the
     * popup should be appear, i.e. the point where the user has clicked.
     * @param windowSize The size that the window will occupy.
     * @return The global screen position where the popup window can be shown.
     */
    virtual QRect calculatePopupWindowPos(const QPoint &absolutePos, const QSize &windowSize) const = 0;
    /**
     * @brief Helper method for calculating the global screen position of a
     * popup window with size windowSize. The parameter plugin should be a
     * plugin
     * @param plugin Plugin that the popup window will belong to. The position
     * will be calculated according to the position of the plugin in the panel.
     * @param windowSize The size that the window will occupy.
     * @return The global screen position where the popup window can be shown.
     */
    virtual QRect calculatePopupWindowPos(const ILXQtPanelPlugin *plugin, const QSize &windowSize) const = 0;
};

#endif // ILXQTPANEL_H
