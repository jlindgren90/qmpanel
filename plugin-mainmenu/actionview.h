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

#if !defined(ACTION_VIEW_H)
#define ACTION_VIEW_H

#include <QListView>

class QStandardItemModel;

//==============================
class StringFilter
{
public:
    StringFilter() = default;
    StringFilter(const QString &searchStr, bool startOfWord);

    const QString& searchStr() const { return searchStr_; }

    bool isMatch(const QString& string) const;

private:
    QString searchStr_;
    QStringList snippets;
    bool startOfWord_ = false;
};
//==============================
#include <QSortFilterProxyModel>
class FilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit FilterProxyModel(QObject* parent = nullptr);
    virtual ~FilterProxyModel();

    void setFilter(const StringFilter& filter) {
        filter_ = filter;
        invalidateFilter();
    }

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const;

private:
    StringFilter filter_;
};
//==============================
class ActionView : public QListView
{
public:
    ActionView(QWidget * parent = nullptr);

    void fillActions(QMenu * menu);
    void setFilter(const StringFilter& filter);
    void activateCurrent();

protected:
    virtual QSize viewportSizeHint() const override;
    virtual QSize minimumSizeHint() const override { return QSize(); }

private:
    void onActivated(QModelIndex const & index);

    QStandardItemModel * mModel;
    FilterProxyModel * mProxy;
};

#endif //ACTION_VIEW_H
