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

// The main panel widget
class LXQtPanel : public QWidget
{
public:
    LXQtPanel();

    QRect calcPopupPos(QPoint const & absolutePos,
                       QSize const & windowSize) const;
    QRect calcPopupPos(QWidget * widget, const QSize & windowSize) const;

protected:
    void showEvent(QShowEvent * event) override
    {
        updateGeometry();
        QWidget::showEvent(event);
    }

private:
    void updateGeometry();

    QPointer<QScreen> mScreen;
    QHBoxLayout mLayout;

    void updateWmStrut();
    void loadPlugins();
    void setPanelGeometry();
};

#endif // LXQTPANEL_H
