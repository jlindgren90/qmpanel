/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2010-2012 Razor team
 * Authors:
 *   Petr Vanek <petr@scribus.info>
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

#include "lxqtquicklaunch.h"
#include "quicklaunchbutton.h"
#include "quicklaunchaction.h"
#include "../panel/ilxqtpanelplugin.h"
#include <QDesktopServices>
#include <QDragEnterEvent>
#include <QFileIconProvider>
#include <QFileInfo>
#include <QLabel>
#include <QMessageBox>
#include <QToolButton>
#include <QUrl>
#include <QDebug>
#include <XdgDesktopFile>
#include <XdgIcon>
#include <LXQt/GridLayout>

LXQtQuickLaunch::LXQtQuickLaunch(ILXQtPanelPlugin *plugin, QWidget* parent) :
    QFrame(parent),
    mPlugin(plugin),
    mPlaceHolder(0)
{
    setAcceptDrops(true);

    mLayout = new LXQt::GridLayout(this);
    setLayout(mLayout);

    QString desktop;
    QString file;
    QString execname;
    QString exec;
    QString icon;

    /* TODO: make this configurable */
    const QStringList apps = {
        "/usr/share/applications/nemo.desktop",
        "/usr/share/applications/xfce4-terminal.desktop",
        "/usr/share/applications/thunderbird.desktop",
        "/usr/share/applications/firefox.desktop",
        "/usr/share/applications/audacious.desktop",
        "/usr/share/applications/org.qt-project.qtcreator.desktop"
    };

    for (const QString &desktop : apps)
    {
        XdgDesktopFile xdg;
        if (!xdg.load(desktop))
        {
            qDebug() << "XdgDesktopFile" << desktop << "is not valid";
            continue;
        }
        if (!xdg.isSuitable())
        {
            qDebug() << "XdgDesktopFile" << desktop << "is not applicable";
            continue;
        }

        addButton(new QuickLaunchAction(&xdg, this));
    } // for

    if (mLayout->isEmpty())
        showPlaceHolder();

    realign();
}


LXQtQuickLaunch::~LXQtQuickLaunch()
{
}


int LXQtQuickLaunch::indexOfButton(QuickLaunchButton* button) const
{
    return mLayout->indexOf(button);
}


int LXQtQuickLaunch::countOfButtons() const
{
    return mLayout->count();
}


void LXQtQuickLaunch::realign()
{
    mLayout->setEnabled(false);
    ILXQtPanel *panel = mPlugin->panel();

    if (mPlaceHolder)
    {
        mLayout->setColumnCount(1);
        mLayout->setRowCount(1);
    }
    else
    {
        if (panel->isHorizontal())
        {
            mLayout->setRowCount(panel->lineCount());
            mLayout->setColumnCount(0);
        }
        else
        {
            mLayout->setColumnCount(panel->lineCount());
            mLayout->setRowCount(0);
        }
    }
    mLayout->setEnabled(true);
}


void LXQtQuickLaunch::addButton(QuickLaunchAction* action)
{
    mLayout->setEnabled(false);
    QuickLaunchButton* btn = new QuickLaunchButton(action, mPlugin, this);
    mLayout->addWidget(btn);

    mLayout->removeWidget(mPlaceHolder);
    delete mPlaceHolder;
    mPlaceHolder = 0;
    mLayout->setEnabled(true);
    realign();
}


void LXQtQuickLaunch::showPlaceHolder()
{
    if (!mPlaceHolder)
    {
        mPlaceHolder = new QLabel(this);
        mPlaceHolder->setAlignment(Qt::AlignCenter);
        mPlaceHolder->setObjectName("QuickLaunchPlaceHolder");
        mPlaceHolder->setText(tr("Drop application\nicons here"));
    }

    mLayout->addWidget(mPlaceHolder);
}
