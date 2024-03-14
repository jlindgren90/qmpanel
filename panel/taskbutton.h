/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * qmpanel - a minimal Qt-based desktop panel
 *
 * Copyright: 2011 Razor team
 *            2014 LXQt team
 *            2020-2024 John Lindgren
 * Authors:
 *   Alexander Sokoloff <sokoloff.a@gmail.com>
 *   Kuzma Shapran <kuzma.shapran@gmail.com>
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

#ifndef TASKBUTTON_H
#define TASKBUTTON_H

#include <QTimer>
#include <QToolButton>

class TaskButton : public QToolButton
{
public:
    QSize sizeHint() const override;

protected:
    TaskButton(QWidget * parent);

    void dragEnterEvent(QDragEnterEvent * event) override;
    void dragLeaveEvent(QDragLeaveEvent * event) override;
    void dropEvent(QDropEvent * event) override;
    void mousePressEvent(QMouseEvent * event) override;

    virtual void activateWindow() = 0;
    virtual void minimizeWindow() = 0;
    virtual void closeWindow() = 0;

private:
    QTimer mTimer;
    bool mHideOnRelease = false;
};

class TaskButtonX11 : public TaskButton
{
public:
    TaskButtonX11(const WId window, QWidget * parent);

    void updateText();
    void updateIcon();

protected:
    void activateWindow() override;
    void minimizeWindow() override;
    void closeWindow() override;

private:
    WId const mWindow;
};

#endif // TASKBUTTON_H
