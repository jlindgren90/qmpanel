/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2016 LXQt team
 * Authors:
 *   Palo Kisa <palo.kisa@gmail.com>
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

#include "actionview.h"

#include <QMenu>
#include <QPointer>
#include <QProxyStyle>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <XdgAction>
#include <algorithm>

class StringFilter
{
public:
    const QString & searchStr() const { return mSearchStr; }

    void setSearchStr(const QString & str)
    {
        mSearchStr = str;
        mSnippets = str.split(' ', QString::SkipEmptyParts);
    }

    bool isMatch(const QString & string) const
    {
        QStringList words = string.split(' ', QString::SkipEmptyParts);

        auto snippetNotFound = [&words](const QString & snippet) {
            auto match = std::find_if(
                words.begin(), words.end(), [&snippet](const QString & word) {
                    return word.startsWith(snippet, Qt::CaseInsensitive);
                });
            return match == words.end();
        };

        return std::find_if(mSnippets.begin(), mSnippets.end(),
                            snippetNotFound) == mSnippets.end();
    }

private:
    QString mSearchStr;
    QStringList mSnippets;
};

class FilterProxyModel : public QSortFilterProxyModel
{
public:
    using QSortFilterProxyModel::QSortFilterProxyModel;

    void setSearchStr(const QString & str)
    {
        mFilter.setSearchStr(str);
        invalidateFilter();
    }

protected:
    bool filterAcceptsRow(int source_row,
                          const QModelIndex & source_parent) const
    {
        if (mFilter.searchStr().isEmpty())
            return true;

        auto srcModel = sourceModel();
        auto index = srcModel->index(source_row, 0, source_parent);
        auto text = srcModel->data(index).toString();
        return mFilter.isMatch(text);
    }

private:
    StringFilter mFilter;
};

class SingleActivateStyle : public QProxyStyle
{
public:
    virtual int styleHint(StyleHint hint, const QStyleOption * option,
                          const QWidget * widget,
                          QStyleHintReturn * returnData) const override
    {
        if (hint == QStyle::SH_ItemView_ActivateItemOnSingleClick)
            return 1;

        return QProxyStyle::styleHint(hint, option, widget, returnData);
    }
};

class ActionItem : public QStandardItem
{
public:
    ActionItem(XdgAction * action) : mAction(action)
    {
        action->updateIcon();
        setIcon(action->icon());
        setText(action->text());
        setToolTip(action->toolTip());
    }

    void trigger()
    {
        if (mAction)
            mAction->trigger();
    }

private:
    QPointer<XdgAction> mAction;
};

ActionView::ActionView(QWidget * parent /*= nullptr*/)
    : QListView(parent), mModel(new QStandardItemModel(this)),
      mProxy(new FilterProxyModel(this))
{
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setFrameStyle(QFrame::NoFrame);
    setSizeAdjustPolicy(AdjustToContents);
    setSelectionBehavior(SelectRows);
    setSelectionMode(SingleSelection);

    SingleActivateStyle * s = new SingleActivateStyle;
    s->setParent(this);
    setStyle(s);

    mProxy->setSourceModel(mModel);
    mProxy->sort(0);
    setModel(mProxy);

    connect(this, &QAbstractItemView::activated, this,
            &ActionView::onActivated);
}

void ActionView::setSearchStr(const QString & str)
{
    mProxy->setSearchStr(str);
    if (mProxy->rowCount() > 0)
        setCurrentIndex(mProxy->index(0, 0));
}

void ActionView::activateCurrent()
{
    QModelIndex const index = currentIndex();
    if (index.isValid())
        emit activated(index);
}

QSize ActionView::viewportSizeHint() const
{
    int count = mProxy->rowCount();
    if (count == 0)
        return QSize();

    return {sizeHintForColumn(0), sizeHintForRow(0) * qMin(count, 10)};
}

void ActionView::onActivated(QModelIndex const & index)
{
    auto realIndex = mProxy->mapToSource(index);
    auto item = static_cast<ActionItem *>(mModel->itemFromIndex(realIndex));
    if (item)
        item->trigger();
}

void ActionView::fillActions(QMenu * menu)
{
    for (auto action : menu->actions())
    {
        if (XdgAction * xdgAction = qobject_cast<XdgAction *>(action))
        {
            mModel->appendRow(new ActionItem(xdgAction));
        }
        else if (QMenu * sub_menu = action->menu())
        {
            fillActions(sub_menu);
        }
    }
}
