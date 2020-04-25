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

class MainMenu : public XdgMenuWidget
{
public:
    MainMenu(const XdgMenu & xdgMenu, Plugin * plugin, QLineEdit * searchEdit, QWidget * parent) :
        XdgMenuWidget(xdgMenu, QString(), parent),
        mPlugin(plugin),
        mSearchEdit(searchEdit) {}

    void inhibitUpdates(void)
    {
        mUpdatesInhibited = true;
    }

    void resumeUpdates(void)
    {
        mUpdatesInhibited = false;
        // force re-layout
        QEvent e(QEvent::StyleChange);
        event(&e);
    }

protected:
    void actionEvent(QActionEvent * e) override
    {
        if(!mUpdatesInhibited)
            XdgMenuWidget::actionEvent(e);
    }

    void keyPressEvent(QKeyEvent * e) override
    {
        if (e->key() == Qt::Key_Escape && !mSearchEdit->text().isEmpty())
            mSearchEdit->setText(QString{});
        else
            XdgMenuWidget::keyPressEvent(e);
    }

    void resizeEvent(QResizeEvent * e) override
    {
        move(mPlugin->calculatePopupWindowPos(e->size()).topLeft());
    }

private:
    Plugin * mPlugin;
    QLineEdit * mSearchEdit;
    bool mUpdatesInhibited = false;
};

LXQtMainMenu::LXQtMainMenu(LXQtPanel *lxqtPanel):
    Plugin(lxqtPanel),
    mMenu(0),
    mSearchEditAction{new QWidgetAction{this}},
    mSearchViewAction{new QWidgetAction{this}}
{
    mDelayedPopup.setSingleShot(true);
    mDelayedPopup.setInterval(200);
    connect(&mDelayedPopup, &QTimer::timeout, this, &LXQtMainMenu::showHideMenu);
    mHideTimer.setSingleShot(true);
    mHideTimer.setInterval(250);

    mButton.setAutoRaise(true);
    mButton.setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

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
    mMenu->inhibitUpdates();
    const bool shown = !text.isEmpty();
    showHideMenuEntries(mMenu, !shown);
    if (shown)
        mSearchView->setFilter(filter);
    mSearchView->setVisible(shown);
    mSearchViewAction->setVisible(shown);
    mMenu->resumeUpdates();
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
    mMenu = new MainMenu(mXdgMenu, this, mSearchEdit, &mButton);

    mMenu->addSeparator();

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
