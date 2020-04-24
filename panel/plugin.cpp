/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2012 Razor team
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


#include "plugin.h"
#include "ilxqtpanelplugin.h"
#include "lxqtpanel.h"
#include <QDebug>
#include <QProcessEnvironment>
#include <QStringList>
#include <QDir>
#include <QFileInfo>
#include <QPluginLoader>
#include <QGridLayout>
#include <QDialog>
#include <QEvent>
#include <QMenu>
#include <QMouseEvent>
#include <QApplication>
#include <QWindow>
#include <memory>

#include <LXQt/Settings>
#include <LXQt/Translator>
#include <XdgIcon>

// statically linked built-in plugins
#include "../plugin-mainmenu/lxqtmainmenu.h" // mainmenu
extern void * loadPluginTranslation_mainmenu_helper;
#include "../plugin-quicklaunch/lxqtquicklaunchplugin.h" // quicklaunch
extern void * loadPluginTranslation_quicklaunch_helper;
#include "../plugin-spacer/spacer.h" // spacer
extern void * loadPluginTranslation_spacer_helper;
#include "../plugin-taskbar/lxqttaskbarplugin.h" // taskbar
extern void * loadPluginTranslation_taskbar_helper;
#include "../plugin-tray/lxqttrayplugin.h" // tray
extern void * loadPluginTranslation_tray_helper;
#include "../plugin-worldclock/lxqtworldclock.h" // worldclock
extern void * loadPluginTranslation_worldclock_helper;

QColor Plugin::mMoveMarkerColor= QColor(255, 0, 0, 255);

/************************************************

 ************************************************/
Plugin::Plugin(ILXQtPanelPlugin *plugin, LXQtPanel *panel) :
    QFrame(panel),
    mPlugin(plugin),
    mPluginWidget(plugin->widget()),
    mPanel(panel)
{
    if (mPluginWidget)
    {
        watchWidgets(mPluginWidget);
    }

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    if (mPluginWidget)
    {
        QGridLayout* layout = new QGridLayout(this);
        layout->setSpacing(0);
        layout->setContentsMargins(0, 0, 0, 0);
        setLayout(layout);
        layout->addWidget(mPluginWidget, 0, 0);
    }
}

/************************************************

 ************************************************/
Plugin::~Plugin()
{
    delete mPlugin;
}

/************************************************

 ************************************************/
void Plugin::watchWidgets(QObject * const widget)
{
    // the QWidget might not be fully constructed yet, but we can rely on the isWidgetType()
    if (!widget->isWidgetType())
        return;
    widget->installEventFilter(this);
    // watch also children (recursive)
    for (auto const & child : widget->children())
    {
        watchWidgets(child);
    }
}

/************************************************

 ************************************************/
void Plugin::unwatchWidgets(QObject * const widget)
{
    widget->removeEventFilter(this);
    // unwatch also children (recursive)
    for (auto const & child : widget->children())
    {
        unwatchWidgets(child);
    }
}

/************************************************

 ************************************************/
void Plugin::settingsChanged()
{
    mPlugin->settingsChanged();
}


/************************************************

 ************************************************/
void Plugin::mousePressEvent(QMouseEvent *event)
{
    switch (event->button())
    {
    case Qt::LeftButton:
        mPlugin->activated(ILXQtPanelPlugin::Trigger);
        break;

    case Qt::MidButton:
        mPlugin->activated(ILXQtPanelPlugin::MiddleClick);
        break;

    default:
        break;
    }
}


/************************************************

 ************************************************/
void Plugin::mouseDoubleClickEvent(QMouseEvent*)
{
    mPlugin->activated(ILXQtPanelPlugin::DoubleClick);
}


/************************************************

 ************************************************/
void Plugin::showEvent(QShowEvent *)
{
    if (mPluginWidget)
        mPluginWidget->adjustSize();
}


/************************************************

 ************************************************/
bool Plugin::isSeparate() const
{
   return mPlugin->isSeparate();
}


/************************************************

 ************************************************/
bool Plugin::isExpandable() const
{
    return mPlugin->isExpandable();
}


/************************************************

 ************************************************/
bool Plugin::eventFilter(QObject * watched, QEvent * event)
{
    switch (event->type())
    {
        case QEvent::ChildAdded:
            watchWidgets(dynamic_cast<QChildEvent *>(event)->child());
            break;
        case QEvent::ChildRemoved:
            unwatchWidgets(dynamic_cast<QChildEvent *>(event)->child());
            break;
        default:
            break;
    }
    return false;
}

/************************************************

 ************************************************/
void Plugin::realign()
{
    if (mPlugin)
        mPlugin->realign();
}
