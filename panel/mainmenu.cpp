/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * qmpanel - a minimal Qt-based desktop panel
 *
 * Copyright: 2010-2011 Razor team
 *            2020 John Lindgren
 * Authors:
 *   Alexander Sokoloff <sokoloff.a@gmail.com>
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

#include "mainmenu.h"
#include "actionview.h"
#include "mainpanel.h"
#include "resources.h"

#include <QHBoxLayout>
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
    MainMenu(Resources & res, QWidget * parent);

protected:
    void keyPressEvent(QKeyEvent * e) override;
    void resizeEvent(QResizeEvent * e) override;
    void showEvent(QShowEvent *) override;

private:
    void populate(Resources & res);
    void searchTextChanged(const QString & text);

    QWidgetAction mSearchEditAction;
    QWidgetAction mSearchViewAction;
    QWidget mSearchFrame;
    QHBoxLayout mSearchLayout;
    QLineEdit mSearchEdit;
    ActionView mSearchView;
    bool mPopulated = false;
};

MainMenu::MainMenu(Resources & res, QWidget * parent)
    : QMenu(parent), mSearchEditAction(this), mSearchViewAction(this),
      mSearchLayout(&mSearchFrame)
{
    mSearchEdit.setPlaceholderText("Search");

    int margin = logicalDpiX() / 32;
    mSearchLayout.setContentsMargins(margin, margin, margin, margin);
    mSearchLayout.addWidget(&mSearchEdit);

    mSearchEditAction.setDefaultWidget(&mSearchFrame);
    mSearchViewAction.setDefaultWidget(&mSearchView);

    mSearchView.hide();
    mSearchViewAction.setVisible(false);

    connect(this, &QMenu::aboutToShow, [this, &res]() { populate(res); });
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

void MainMenu::keyPressEvent(QKeyEvent * e)
{
    if (e->key() == Qt::Key_Escape && !mSearchEdit.text().isEmpty())
        mSearchEdit.clear();
    else
        QMenu::keyPressEvent(e);
}

void MainMenu::resizeEvent(QResizeEvent * e)
{
    // anchor the bottom edge
    if (isVisible())
        move(x(), y() + e->oldSize().height() - height());
}

void MainMenu::showEvent(QShowEvent *)
{
    mSearchEdit.setFocus(Qt::OtherFocusReason);
}

void MainMenu::populate(Resources & res)
{
    if (mPopulated)
        return;

    static const Category categories[] = {
        {"applications-development", "Development", "Development"},
        {"applications-science", "Education", "Education"},
        {"applications-games", "Games", "Game"},
        {"applications-graphics", "Graphics", "Graphics"},
        {"applications-multimedia", "Multimedia", "AudioVideo"},
        {"applications-internet", "Network", "Network"},
        {"applications-office", "Office", "Office"},
        {"preferences-desktop", "Settings", "Settings"},
        {"applications-system", "System", "System"},
        {"applications-accessories", "Utility", "Utility"}};

    std::unordered_set<QString> added;
    for (auto app : res.settings().pinnedMenuApps)
    {
        auto action = res.getAction(app);
        if (action)
        {
            addAction(action);
            added.insert(app);
        }
    }

    addSeparator();

    for (auto & category : categories)
    {
        auto apps = res.getCategory(category.internalName, added);
        if (!apps.isEmpty())
        {
            auto icon = res.getIcon(category.icon);
            addMenu(icon, category.displayName)->addActions(apps);
            mSearchView.addActions(apps);
        }
    }

    addAction(&mSearchViewAction);
    addAction(&mSearchEditAction);

    mPopulated = true;
}

void MainMenu::searchTextChanged(const QString & text)
{
    bool shown = !text.isEmpty();

    for (auto const & action : actions())
    {
        if (qobject_cast<QWidgetAction *>(action) == nullptr)
            action->setVisible(!shown);
    }

    if (shown)
        mSearchView.setSearchStr(text);

    mSearchView.setVisible(shown);
    mSearchViewAction.setVisible(shown);

    // force re-layout
    QEvent e(QEvent::StyleChange);
    event(&e);
}

MainMenuButton::MainMenuButton(Resources & res, MainPanel * panel)
    : QToolButton(panel)
{
    auto menu = new MainMenu(res, this);
    panel->registerMenu(menu);

    setAutoRaise(true);
    setIcon(res.getIcon(res.settings().menuIcon));
    setMenu(menu);
    setPopupMode(InstantPopup);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    setStyleSheet("QToolButton::menu-indicator { image: none; }");
    setToolButtonStyle(Qt::ToolButtonIconOnly);
}
