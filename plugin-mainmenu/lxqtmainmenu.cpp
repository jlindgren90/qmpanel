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
#include "actionview.h"

#include <QKeyEvent>
#include <QLineEdit>
#include <QResizeEvent>
#include <QWidgetAction>
#include <XdgMenuWidget>

class MainMenu : public XdgMenuWidget
{
public:
    MainMenu(const XdgMenu & xdgMenu, LXQtMainMenu * plugin, QWidget * parent);

protected:
    void actionEvent(QActionEvent * e) override;
    void keyPressEvent(QKeyEvent * e) override;
    void resizeEvent(QResizeEvent * e) override;
    void showEvent(QShowEvent *) override;

private:
    void searchTextChanged(const QString & text);

    LXQtMainMenu * mPlugin;
    QLineEdit mSearchEdit;
    ActionView mSearchView;
    QWidgetAction mSearchEditAction;
    QWidgetAction mSearchViewAction;
    bool mUpdatesInhibited = false;
};

MainMenu::MainMenu(const XdgMenu & xdgMenu, LXQtMainMenu * plugin,
                   QWidget * parent)
    : XdgMenuWidget(xdgMenu, QString(), parent), mPlugin(plugin),
      mSearchEditAction(this), mSearchViewAction(this)
{
    mSearchEdit.setClearButtonEnabled(true);
    mSearchEdit.setPlaceholderText("Search");
    mSearchView.setMaxItemsToShow(10);
    mSearchView.setMaxItemWidth(300); /* TODO: scale by DPI */
    mSearchView.fillActions(this);

    mSearchEditAction.setDefaultWidget(&mSearchEdit);
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
        XdgMenuWidget::actionEvent(e);
}

void MainMenu::keyPressEvent(QKeyEvent * e)
{
    if (e->key() == Qt::Key_Escape && !mSearchEdit.text().isEmpty())
        mSearchEdit.clear();
    else
        XdgMenuWidget::keyPressEvent(e);
}

void MainMenu::resizeEvent(QResizeEvent * e)
{
    move(mPlugin->calculatePopupWindowPos(e->size()).topLeft());
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
        mSearchView.setFilter(StringFilter(text, true));

    mSearchView.setVisible(shown);
    mSearchViewAction.setVisible(shown);
    mUpdatesInhibited = false;

    // force re-layout
    QEvent e(QEvent::StyleChange);
    event(&e);
}

LXQtMainMenu::LXQtMainMenu(LXQtPanel * lxqtPanel) : Plugin(lxqtPanel)
{
    mButton.setAutoRaise(true);
    mButton.setIcon(QIcon("/usr/share/pixmaps/j-login.png"));
    mButton.setToolButtonStyle(Qt::ToolButtonIconOnly);

    mXdgMenu.setEnvironments(QStringList() << "X-LXQT"
                                           << "LXQt");
    /* TODO: rework menu */
    mXdgMenu.read("/home/john/.config/menus/programs.menu");

    mMenu = new MainMenu(mXdgMenu, this, &mButton);

    connect(&mButton, &QToolButton::clicked, [this]() {
        if (mMenu->isVisible())
            mMenu->hide();
        else
            mMenu->popup(calculatePopupWindowPos(mMenu->sizeHint()).topLeft());
    });
}

LXQtMainMenu::~LXQtMainMenu() { delete mMenu; }
