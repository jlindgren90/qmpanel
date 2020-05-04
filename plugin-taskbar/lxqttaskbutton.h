/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2011 Razor team
 *            2014 LXQt team
 * Authors:
 *   Alexander Sokoloff <sokoloff.a@gmail.com>
 *   Kuzma Shapran <kuzma.shapran@gmail.com>
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


#ifndef LXQTTASKBUTTON_H
#define LXQTTASKBUTTON_H

#include <QToolButton>

class LXQtPanel;

class LXQtTaskButton : public QToolButton
{
    Q_OBJECT

public:
    explicit LXQtTaskButton(const WId window, LXQtPanel * panel, QWidget *parent = 0);
    virtual ~LXQtTaskButton();

    bool isApplicationHidden() const;
    bool isApplicationActive() const;
    WId windowId() const { return mWindow; }

    bool isOnDesktop(int desktop) const;
    bool isMinimized() const;
    void updateText();
    void updateIcon();

    QSize sizeHint() const override;

public slots:
    void raiseApplication();
    void minimizeApplication();
    void maximizeApplication();
    void deMaximizeApplication();
    void shadeApplication();
    void unShadeApplication();
    void closeApplication();
    void moveApplicationToDesktop();
    void moveApplication();
    void resizeApplication();
    void setApplicationLayer();

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

private:
    WId const mWindow;
    LXQtPanel * const mPanel;
    int mIconSize;

    // Timer for when draggind something into a button (the button's window
    // must be activated so that the use can continue dragging to the window
    QTimer * mDNDTimer;

private slots:
    void activateWithDraggable();

signals:
    void dropped(QObject * dragSource, QPoint const & pos);
    void dragging(QObject * dragSource, QPoint const & pos);
};

typedef QHash<WId,LXQtTaskButton*> LXQtTaskButtonHash;

#endif // LXQTTASKBUTTON_H
