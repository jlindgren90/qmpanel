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


#include "lxqtmainmenu.h"
#include "../panel/lxqtpanel.h"
#include "actionview.h"
#include <QAction>
#include <QTimer>
#include <QMessageBox>
#include <QEvent>
#include <QKeyEvent>
#include <QResizeEvent>
#include <QWidgetAction>
#include <QLineEdit>
#include <lxqt-globalkeys.h>
#include <algorithm> // for find_if()
#include <KWindowSystem/KWindowSystem>
#include <QApplication>

#include <XdgMenuWidget>
#include <XdgAction>

#define DEFAULT_SHORTCUT "Alt+F1"

LXQtMainMenu::LXQtMainMenu(LXQtPanel *lxqtPanel):
    Plugin(lxqtPanel),
    mMenu(0),
    mSearchEditAction{new QWidgetAction{this}},
    mSearchViewAction{new QWidgetAction{this}},
    mMakeDirtyAction{new QAction{this}},
    mFilterMenu(true),
    mFilterShow(true),
    mFilterStartOfWord(false),
    mFilterClear(false),
    mFilterShowHideMenu(true),
    mHeavyMenuChanges(false)
{
    mDelayedPopup.setSingleShot(true);
    mDelayedPopup.setInterval(200);
    connect(&mDelayedPopup, &QTimer::timeout, this, &LXQtMainMenu::showHideMenu);
    mHideTimer.setSingleShot(true);
    mHideTimer.setInterval(250);

    mButton.setAutoRaise(true);
    mButton.setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    //Notes:
    //1. installing event filter to parent widget to avoid infinite loop
    //   (while setting icon we also need to set the style)
    //2. delaying of installEventFilter because in c-tor mButton has no parent widget
    //   (parent is assigned in panel's logic after widget() call)
    QTimer::singleShot(0, [this] { Q_ASSERT(mButton.parentWidget()); mButton.parentWidget()->installEventFilter(this); });

    connect(&mButton, &QToolButton::clicked, this, &LXQtMainMenu::showHideMenu);

    mSearchView = new ActionView;
    mSearchView->setVisible(false);
    connect(mSearchView, &QAbstractItemView::activated, this, &LXQtMainMenu::showHideMenu);
    mSearchViewAction->setDefaultWidget(mSearchView);
    mSearchEdit = new QLineEdit;
    mSearchEdit->setClearButtonEnabled(true);
    mSearchEdit->setPlaceholderText(LXQtMainMenu::tr("Search..."));
    connect(mSearchEdit, &QLineEdit::textChanged, this, &LXQtMainMenu::searchTextChanged);
    connect(mSearchEdit, &QLineEdit::returnPressed, mSearchView, &ActionView::activateCurrent);
    mSearchEditAction->setDefaultWidget(mSearchEdit);
    QTimer::singleShot(0, [this] { settingsChanged(); });
}


/************************************************

 ************************************************/
LXQtMainMenu::~LXQtMainMenu()
{
    mButton.parentWidget()->removeEventFilter(this);
    if (mMenu)
    {
        mMenu->removeAction(mSearchEditAction);
        mMenu->removeAction(mSearchViewAction);
        delete mMenu;
    }
}

/************************************************

 ************************************************/
void LXQtMainMenu::showHideMenu()
{
    if (mMenu && mMenu->isVisible())
        mMenu->hide();
    else
        showMenu();
}

/************************************************

 ************************************************/
void LXQtMainMenu::showMenu()
{
    if (!mMenu)
        return;

    // Just using Qt`s activateWindow() won't work on some WMs like Kwin.
    // Solution is to execute menu 1ms later using timer
    mMenu->popup(calculatePopupWindowPos(mMenu->sizeHint()).topLeft());
    if (mFilterMenu || mFilterShow)
        mSearchEdit->setFocus();
}

/************************************************

 ************************************************/
void LXQtMainMenu::settingsChanged()
{
    setButtonIcon();
    mButton.setText("");
    mButton.setToolButtonStyle(Qt::ToolButtonIconOnly);
    mLogDir = "";

    QString menu_file = "/home/john/.config/menus/programs.menu"; /* TODO: rework menu */
    if (menu_file.isEmpty())
        menu_file = XdgMenu::getMenuFileName();

    if (mMenuFile != menu_file)
    {
        mMenuFile = menu_file;
        mXdgMenu.setEnvironments(QStringList() << "X-LXQT" << "LXQt");
        mXdgMenu.setLogDir(mLogDir);

        bool res = mXdgMenu.read(mMenuFile);
        connect(&mXdgMenu, &XdgMenu::changed, this, &LXQtMainMenu::buildMenu);
        if (res)
        {
            QTimer::singleShot(1000, this, &LXQtMainMenu::buildMenu);
        }
        else
        {
            QMessageBox::warning(0, "Parse error", mXdgMenu.errorString());
            return;
        }
    }

    //clear the search to not leaving the menu in wrong state
    mSearchEdit->setText(QString{});
    mFilterMenu = true;
    mFilterShow = true;
    mFilterStartOfWord = true;
    mFilterClear = true;
    mFilterShowHideMenu = true;
    if (mMenu)
    {
        mSearchEdit->setVisible(mFilterMenu || mFilterShow);
        mSearchEditAction->setVisible(mFilterMenu || mFilterShow);
        if (mFilterClear && !mMenu->isVisible())
            mSearchEdit->clear();
    }
    mSearchView->setMaxItemsToShow(10);
    mSearchView->setMaxItemWidth(300); /* TODO: scale by DPI */

    realign();
}

static bool filterMenu(QMenu * menu, const StringFilter& filter)
{
    bool has_visible = false;
    const auto actions = menu->actions();
    for (auto const & action : actions)
    {
        if (QMenu * sub_menu = action->menu())
        {
            action->setVisible(filterMenu(sub_menu, filter)/*recursion*/);
            has_visible |= action->isVisible();
        } else if (nullptr != qobject_cast<QWidgetAction *>(action))
        {
            //our searching widget
            has_visible = true;
        } else if (!action->isSeparator())
        {
            //real menu action -> app
            bool visible(filter.searchStr().isEmpty() || filter.isMatch(action->text()) || filter.isMatch(action->toolTip()));
            if(!visible)
            {
                if (XdgAction * xdgAction = qobject_cast<XdgAction *>(action))
                {
                    const XdgDesktopFile& df = xdgAction->desktopFile();
                    QStringList list = df.expandExecString();
                    if (!list.isEmpty())
                    {
                        if (filter.isMatch(list.at(0)))
                            visible = true;
                    }
                }
            }
            action->setVisible(visible);
            has_visible |= action->isVisible();
        }
    }
    return has_visible;
}

static void showHideMenuEntries(QMenu * menu, bool show)
{
    //show/hide the top menu entries
    const auto actions = menu->actions();
    for (auto const & action : actions)
    {
        if (nullptr == qobject_cast<QWidgetAction *>(action))
        {
            action->setVisible(show);
        }
    }
}

static void setTranslucentMenus(QMenu * menu)
{
    menu->setAttribute(Qt::WA_TranslucentBackground);
    const auto actions = menu->actions();
    for (auto const & action : actions)
    {
        if (QMenu * sub_menu = action->menu())
        {
            setTranslucentMenus(sub_menu);
        }
    }
}

/************************************************

 ************************************************/
void LXQtMainMenu::searchTextChanged(QString const & text)
{
    StringFilter filter(text, mFilterStartOfWord);
    if (mFilterShow)
    {
        mHeavyMenuChanges = true;
        const bool shown = !text.isEmpty();
        if (mFilterShowHideMenu)
            showHideMenuEntries(mMenu, !shown);
        if (shown)
            mSearchView->setFilter(filter);
        mSearchView->setVisible(shown);
        mSearchViewAction->setVisible(shown);
        //TODO: how to force the menu to recalculate it's size in a more elegant way?
        mMenu->addAction(mMakeDirtyAction);
        mMenu->removeAction(mMakeDirtyAction);
        mHeavyMenuChanges = false;
    }
    if (mFilterMenu && !(mFilterShow && mFilterShowHideMenu))
        filterMenu(mMenu, filter);

}

/************************************************

 ************************************************/
void LXQtMainMenu::setSearchFocus(QAction *action)
{
    if (mFilterMenu || mFilterShow)
    {
        if(action == mSearchEditAction)
            mSearchEdit->setFocus();
        else
            mSearchEdit->clearFocus();
    }
}

static void menuInstallEventFilter(QMenu * menu, QObject * watcher)
{
    for (auto const & action : const_cast<QList<QAction *> const &&>(menu->actions()))
    {
        if (action->menu())
            menuInstallEventFilter(action->menu(), watcher); // recursion
    }
    menu->installEventFilter(watcher);
}

/************************************************

 ************************************************/
void LXQtMainMenu::buildMenu()
{
    if(mMenu)
    {
        mMenu->removeAction(mSearchEditAction);
        mMenu->removeAction(mSearchViewAction);
        delete mMenu;
    }
    mMenu = new XdgMenuWidget(mXdgMenu, "", &mButton);
    mMenu->setObjectName("TopLevelMainMenu");
    setTranslucentMenus(mMenu);

    mMenu->addSeparator();

    menuInstallEventFilter(mMenu, this);
    connect(mMenu, &QMenu::aboutToHide, &mHideTimer, static_cast<void (QTimer::*)()>(&QTimer::start));
    connect(mMenu, &QMenu::aboutToShow, &mHideTimer, &QTimer::stop);

    mMenu->addSeparator();
    mMenu->addAction(mSearchViewAction);
    mMenu->addAction(mSearchEditAction);
    connect(mMenu, &QMenu::hovered, this, &LXQtMainMenu::setSearchFocus);
    connect(mMenu, &QMenu::aboutToHide, [this] {
        if (mFilterClear)
            mSearchEdit->clear();
    });
    mSearchEdit->setVisible(mFilterMenu || mFilterShow);
    mSearchEditAction->setVisible(mFilterMenu || mFilterShow);
    mSearchView->fillActions(mMenu);

    searchTextChanged(mSearchEdit->text());
}

/************************************************

 ************************************************/
void LXQtMainMenu::setButtonIcon()
{
    /* TODO: make configurable */
    mButton.setIcon(QIcon("/usr/share/pixmaps/j-login.png"));
}

/************************************************

 ************************************************/

// functor used to match a QAction by prefix
struct MatchAction
{
    MatchAction(QString key):key_(key) {}
    bool operator()(QAction* action) { return action->text().startsWith(key_, Qt::CaseInsensitive); }
    QString key_;
};

bool LXQtMainMenu::eventFilter(QObject *obj, QEvent *event)
{
    if(QMenu* menu = qobject_cast<QMenu*>(obj))
    {
        if(event->type() == QEvent::KeyPress)
        {
            // if our shortcut key is pressed while the menu is open, close the menu
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
            if (keyEvent->modifiers() & ~Qt::ShiftModifier)
            {
                mHideTimer.start();
                mMenu->hide(); // close the app menu
                return true;
            }
            else // go to the menu item which starts with the pressed key if there is an active action.
            {
                QString key = keyEvent->text();
                if(key.isEmpty())
                    return false;
                QAction* action = menu->activeAction();
                if(action !=0) {
                    QList<QAction*> actions = menu->actions();
                    QList<QAction*>::iterator it = qFind(actions.begin(), actions.end(), action);
                    it = std::find_if(it + 1, actions.end(), MatchAction(key));
                    if(it == actions.end())
                        it = std::find_if(actions.begin(), it, MatchAction(key));
                    if(it != actions.end())
                        menu->setActiveAction(*it);
                }
            }
        }

        if (obj == mMenu)
        {
            if (event->type() == QEvent::Resize)
            {
                QResizeEvent * e = dynamic_cast<QResizeEvent *>(event);
                if (e->oldSize().isValid() && e->oldSize() != e->size())
                {
                    mMenu->move(calculatePopupWindowPos(e->size()).topLeft());
                }
            } else if (event->type() == QEvent::KeyPress)
            {
                QKeyEvent * e = dynamic_cast<QKeyEvent*>(event);
                if (Qt::Key_Escape == e->key())
                {
                    if (!mSearchEdit->text().isEmpty())
                    {
                        mSearchEdit->setText(QString{});
                        //filter out this to not close the menu
                        return true;
                    }
                }
            } else if (QEvent::ActionChanged == event->type()
                    || QEvent::ActionAdded == event->type())
            {
                //filter this if we are performing heavy changes to reduce flicker
                if (mHeavyMenuChanges)
                    return true;
            }
        }
    }
    return false;
}

#undef DEFAULT_SHORTCUT
