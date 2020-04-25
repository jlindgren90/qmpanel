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


#ifndef LXQT_MAINMENU_H
#define LXQT_MAINMENU_H

#include "../panel/plugin.h"
#include <XdgMenu>

#include <QLabel>
#include <QToolButton>
#include <QDomElement>
#include <QAction>
#include <QTimer>
#include <QKeySequence>

class QMenu;
class QWidgetAction;
class QLineEdit;
class ActionView;
class LXQtBar;

namespace LXQt {
class PowerManager;
class ScreenSaver;
}

namespace GlobalKeyShortcut
{
class Action;
}

class MainMenu;

class LXQtMainMenu : public Plugin
{
    Q_OBJECT
public:
    LXQtMainMenu(LXQtPanel *lxqtPanel);
    ~LXQtMainMenu();

    QWidget *widget() { return &mButton; }

private:
    void setButtonIcon();

private:
    QToolButton mButton;
    QString mLogDir;
    MainMenu * mMenu;
    QWidgetAction * mSearchEditAction;
    QLineEdit * mSearchEdit;
    QWidgetAction * mSearchViewAction;
    ActionView * mSearchView;

    XdgMenu mXdgMenu;

    QTimer mDelayedPopup;
    QTimer mHideTimer;
    QString mMenuFile;

protected slots:

    virtual void settingsChanged();
    void buildMenu();

private slots:
    void showMenu();
    void showHideMenu();
    void searchTextChanged(QString const & text);
    void setSearchFocus(QAction *action);
};

#endif
