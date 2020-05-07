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

#include "mainmenu.h"
#include "actionview.h"
#include "appdb.h"

#include "../panel/lxqtpanel.h"

#include <QKeyEvent>
#include <QLineEdit>
#include <QMenu>
#include <QResizeEvent>
#include <QWidgetAction>

struct Category
{
    const char * icon;
    const char * displayName;
    const char * internalName;
};

class MainMenu : public QMenu
{
public:
    MainMenu(const AppDB & appDB, MainMenuButton * button);

protected:
    void actionEvent(QActionEvent * e) override;
    void keyPressEvent(QKeyEvent * e) override;
    void resizeEvent(QResizeEvent * e) override;
    void showEvent(QShowEvent *) override;

private:
    void searchTextChanged(const QString & text);

    MainMenuButton * const mButton;
    QWidgetAction mSearchEditAction;
    QWidgetAction mSearchViewAction;
    QWidget mSearchFrame;
    QHBoxLayout mSearchLayout;
    QLineEdit mSearchEdit;
    ActionView mSearchView;
    bool mUpdatesInhibited = false;
};

MainMenu::MainMenu(const AppDB & appDB, MainMenuButton * button)
    : QMenu(button), mButton(button), mSearchEditAction(this),
      mSearchViewAction(this), mSearchLayout(&mSearchFrame)
{
    /* TODO: make configurable */
    static const char * pinnedApps[] = {"logout.desktop", "run.desktop"};

    /* TODO: make configurable */
    static const Category categories[] = {
        {"applications-accessories", "Accessories", "Utility"},
        {"applications-development", "Development", "Development"},
        {"applications-science", "Education", "Education"},
        {"applications-games", "Games", "Game"},
        {"applications-graphics", "Graphics", "Graphics"},
        {"applications-multimedia", "Multimedia", "AudioVideo"},
        {"applications-internet", "Internet", "Network"},
        {"applications-office", "Office", "Office"},
        {"preferences-desktop", "Settings", "Settings"},
        {"applications-system", "System", "System"}};

    for (auto app : pinnedApps)
    {
        auto action = appDB.createAction(app, this);
        if (action)
            addAction(action);
    }

    addSeparator();

    for (auto & category : categories)
    {
        auto apps = appDB.createCategory(category.internalName, this);
        if (!apps.isEmpty())
        {
            auto icon = QIcon::fromTheme(category.icon);
            addMenu(icon, category.displayName)->addActions(apps);
            mSearchView.addActions(apps);
        }
    }

    mSearchEdit.setClearButtonEnabled(true);
    mSearchEdit.setPlaceholderText("Search");
    mSearchLayout.setContentsMargins(3, 3, 3, 3); /* TODO: scale by DPI */
    mSearchLayout.addWidget(&mSearchEdit);

    mSearchEditAction.setDefaultWidget(&mSearchFrame);
    mSearchViewAction.setDefaultWidget(&mSearchView);
    addAction(&mSearchViewAction);
    addAction(&mSearchEditAction);

    mSearchView.hide();
    mSearchViewAction.setVisible(false);

    connect(this, &QMenu::aboutToHide, &mSearchEdit, &QLineEdit::clear);
    connect(this, &QMenu::hovered, [this](QAction * action) {
        if (action == &mSearchEditAction)
            mSearchEdit.setFocus();
        else
            mSearchEdit.clearFocus();
    });

    connect(&mSearchEdit, &QLineEdit::textChanged, this,
            &MainMenu::searchTextChanged);
    connect(&mSearchEdit, &QLineEdit::returnPressed, &mSearchView,
            &ActionView::activateCurrent);
    connect(&mSearchView, &QListView::activated, this, &QMenu::hide);
}

void MainMenu::actionEvent(QActionEvent * e)
{
    if (!mUpdatesInhibited)
        QMenu::actionEvent(e);
}

void MainMenu::keyPressEvent(QKeyEvent * e)
{
    if (e->key() == Qt::Key_Escape && !mSearchEdit.text().isEmpty())
        mSearchEdit.clear();
    else
        QMenu::keyPressEvent(e);
}

void MainMenu::resizeEvent(QResizeEvent * e)
{
    move(mButton->panel()->calcPopupPos(mButton, e->size()).topLeft());
}

void MainMenu::showEvent(QShowEvent *)
{
    mSearchEdit.setFocus(Qt::OtherFocusReason);
}

void MainMenu::searchTextChanged(const QString & text)
{
    bool shown = !text.isEmpty();

    mUpdatesInhibited = true;

    for (auto const & action : actions())
    {
        if (qobject_cast<QWidgetAction *>(action) == nullptr)
            action->setVisible(!shown);
    }

    if (shown)
        mSearchView.setSearchStr(text);

    mSearchView.setVisible(shown);
    mSearchViewAction.setVisible(shown);
    mUpdatesInhibited = false;

    // force re-layout
    QEvent e(QEvent::StyleChange);
    event(&e);
}

MainMenuButton::MainMenuButton(const AppDB & appDB, LXQtPanel * panel)
    : QToolButton(panel), mPanel(panel), mMenu(new MainMenu(appDB, this))
{
    setAutoRaise(true);
    /* TODO: make configurable */
    setIcon(QIcon("/usr/share/pixmaps/j-login.png"));
    setToolButtonStyle(Qt::ToolButtonIconOnly);

    connect(this, &QToolButton::clicked, [this]() {
        if (mMenu->isVisible())
            mMenu->hide();
        else
        {
            auto pos = mPanel->calcPopupPos(this, mMenu->sizeHint());
            mMenu->popup(pos.topLeft());
        }
    });
}
