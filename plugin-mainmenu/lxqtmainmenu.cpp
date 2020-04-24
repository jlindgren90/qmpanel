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

    mMenu->popup(calculatePopupWindowPos(mMenu->sizeHint()).topLeft());
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

    mSearchView->setMaxItemsToShow(10);
    mSearchView->setMaxItemWidth(300); /* TODO: scale by DPI */
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

/************************************************

 ************************************************/
void LXQtMainMenu::searchTextChanged(QString const & text)
{
    StringFilter filter(text, true);
    mHeavyMenuChanges = true;
    const bool shown = !text.isEmpty();
    showHideMenuEntries(mMenu, !shown);
    if (shown)
        mSearchView->setFilter(filter);
    mSearchView->setVisible(shown);
    mSearchViewAction->setVisible(shown);
    mHeavyMenuChanges = false;
    // force the menu to recalculate its size
    QApplication::postEvent(mMenu, new QEvent(QEvent::StyleChange));
}

/************************************************

 ************************************************/
void LXQtMainMenu::setSearchFocus(QAction *action)
{
    if(action == mSearchEditAction)
        mSearchEdit->setFocus();
    else
        mSearchEdit->clearFocus();
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

    mMenu->addSeparator();

    menuInstallEventFilter(mMenu, this);
    connect(mMenu, &QMenu::aboutToHide, &mHideTimer, static_cast<void (QTimer::*)()>(&QTimer::start));
    connect(mMenu, &QMenu::aboutToShow, &mHideTimer, &QTimer::stop);

    mMenu->addSeparator();
    mMenu->addAction(mSearchViewAction);
    mMenu->addAction(mSearchEditAction);
    connect(mMenu, &QMenu::hovered, this, &LXQtMainMenu::setSearchFocus);
    connect(mMenu, &QMenu::aboutToHide, [this] {
        mSearchEdit->clear();
    });
    mSearchEdit->setVisible(true);
    mSearchEditAction->setVisible(true);
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
    if(qobject_cast<QMenu*>(obj))
    {
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
