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
 *   Maciej Płaza <plaza.maciej@gmail.com>
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


#ifndef LXQTTASKBAR_H
#define LXQTTASKBAR_H

#include "../panel/plugin.h"
#include "lxqttaskbutton.h"

#include <QFrame>
#include <QBoxLayout>
#include <QMap>
#include <lxqt-globalkeys.h>
#include <KWindowSystem/KWindowSystem>
#include <KWindowSystem/KWindowInfo>
#include <KWindowSystem/NETWM>

class QSignalMapper;
class LXQtTaskButton;
class ElidedButtonStyle;

namespace LXQt {
class GridLayout;
}

class LXQtTaskBar : public QFrame
{
    Q_OBJECT

public:
    explicit LXQtTaskBar(Plugin *plugin, QWidget* parent = 0);

    void realign();

    int buttonWidth() const { return mButtonWidth; }
    inline LXQtPanel * panel() const { return mPlugin->panel(); }
    inline Plugin * plugin() const { return mPlugin; }

public slots:
    void settingsChanged();

signals:
    void buttonStyleRefreshed(Qt::ToolButtonStyle buttonStyle);

protected:
    virtual void dragEnterEvent(QDragEnterEvent * event);
    virtual void dragMoveEvent(QDragMoveEvent * event);

private slots:
    void refreshTaskList();
    void onWindowAdded(WId window);
    void onWindowRemoved(WId window);
    void onActiveWindowChanged(WId window);
    void activateTask(int pos);

private:
    typedef QMap<WId, LXQtTaskButton*> windowMap_t;

private:
    void addWindow(WId window);
    windowMap_t::iterator removeWindow(windowMap_t::iterator pos);
    void buttonMove(LXQtTaskButton * dst, LXQtTaskButton * src, QPoint const & pos);

private:
    QMap<WId, LXQtTaskButton*> mKnownWindows; //!< Ids of known windows (mapping to buttons/groups)
    LXQt::GridLayout *mLayout;
    QList<GlobalKeyShortcut::Action*> mKeys;
    QSignalMapper *mSignalMapper;

    // Settings
    int mButtonWidth;
    int mButtonHeight;

    bool acceptWindow(WId window) const;

    Plugin *mPlugin;
};

#endif // LXQTTASKBAR_H
