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


#include "lxqtpanel.h"
#include "lxqtpanellimits.h"
#include "ilxqtpanelplugin.h"
#include "lxqtpanelapplication.h"
#include "popupmenu.h"
#include "plugin.h"

#include "../plugin-mainmenu/lxqtmainmenu.h"
#include "../plugin-quicklaunch/lxqtquicklaunchplugin.h"
#include "../plugin-taskbar/lxqttaskbarplugin.h"
#include "../plugin-tray/lxqttrayplugin.h"
#include "../plugin-worldclock/lxqtworldclock.h"

#include <QFileInfo>
#include <QScreen>
#include <QWindow>
#include <QX11Info>
#include <QDebug>
#include <QString>
#include <QDesktopWidget>
#include <QMenu>
#include <QMessageBox>
#include <QDropEvent>
#include <XdgIcon>
#include <XdgDirs>

#include <KWindowSystem/KWindowSystem>
#include <KWindowSystem/NETWM>

// Turn on this to show the time required to load each plugin during startup
// #define DEBUG_PLUGIN_LOADTIME
#ifdef DEBUG_PLUGIN_LOADTIME
#include <QElapsedTimer>
#endif

// Config keys and groups
#define CFG_KEY_SCREENNUM          "desktop"
#define CFG_KEY_POSITION           "position"
#define CFG_KEY_PANELSIZE          "panelSize"
#define CFG_KEY_ICONSIZE           "iconSize"
#define CFG_KEY_LINECNT            "lineCount"
#define CFG_KEY_LENGTH             "width"
#define CFG_KEY_PERCENT            "width-percent"
#define CFG_KEY_ALIGNMENT          "alignment"
#define CFG_KEY_FONTCOLOR          "font-color"
#define CFG_KEY_BACKGROUNDCOLOR    "background-color"
#define CFG_KEY_BACKGROUNDIMAGE    "background-image"
#define CFG_KEY_OPACITY            "opacity"
#define CFG_KEY_RESERVESPACE       "reserve-space"
#define CFG_KEY_PLUGINS            "plugins"
#define CFG_KEY_HIDABLE            "hidable"
#define CFG_KEY_VISIBLE_MARGIN     "visible-margin"
#define CFG_KEY_ANIMATION          "animation-duration"
#define CFG_KEY_SHOW_DELAY         "show-delay"
#define CFG_KEY_LOCKPANEL          "lockPanel"

/************************************************

 ************************************************/
LXQtPanel::LXQtPanel(const QString &configGroup, LXQt::Settings *settings, QWidget *parent) :
    QFrame(parent),
    mSettings(settings),
    mConfigGroup(configGroup),
    mScreenNum(0), //whatever (avoid conditional on uninitialized value)
    mActualScreenNum(0)
{
    //You can find information about the flags and widget attributes in your
    //Qt documentation or at https://doc.qt.io/qt-5/qt.html
    //Qt::FramelessWindowHint = Produces a borderless window. The user cannot
    //move or resize a borderless window via the window system. On X11, ...
    //Qt::WindowStaysOnTopHint = Informs the window system that the window
    //should stay on top of all other windows. Note that on ...
    Qt::WindowFlags flags = Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint;

    // NOTE: by PCMan:
    // In Qt 4, the window is not activated if it has Qt::WA_X11NetWmWindowTypeDock.
    // Since Qt 5, the default behaviour is changed. A window is always activated on mouse click.
    // Please see the source code of Qt5: src/plugins/platforms/xcb/qxcbwindow.cpp.
    // void QXcbWindow::handleButtonPressEvent(const xcb_button_press_event_t *event)
    // This new behaviour caused lxqt bug #161 - Cannot minimize windows from panel 1 when two task managers are open
    // Besides, this breaks minimizing or restoring windows when clicking on the taskbar buttons.
    // To workaround this regression bug, we need to add this window flag here.
    // However, since the panel gets no keyboard focus, this may decrease accessibility since
    // it's not possible to use the panel with keyboards. We need to find a better solution later.
    flags |= Qt::WindowDoesNotAcceptFocus;

    setWindowFlags(flags);
    //Adds _NET_WM_WINDOW_TYPE_DOCK to the window's _NET_WM_WINDOW_TYPE X11 window property. See https://standards.freedesktop.org/wm-spec/ for more details.
    setAttribute(Qt::WA_X11NetWmWindowTypeDock);
    //Enables tooltips for inactive windows.
    setAttribute(Qt::WA_AlwaysShowToolTips);
    //Allows data from drag and drop operations to be dropped onto the widget (see QWidget::setAcceptDrops()).
    setAttribute(Qt::WA_AcceptDrops);

    setWindowTitle("LXQt Panel");
    setObjectName(QString("LXQtPanel %1").arg(configGroup));

    //LXQtPanel (inherits QFrame) -> lav (QGridLayout) -> LXQtPanelWidget (QFrame) -> LXQtPanelLayout
    LXQtPanelWidget = new QFrame(this);
    LXQtPanelWidget->setObjectName("BackgroundWidget");
    QGridLayout* lav = new QGridLayout();
    lav->setContentsMargins(0, 0, 0, 0);
    setLayout(lav);
    this->layout()->addWidget(LXQtPanelWidget);

    mLayout = new QHBoxLayout(LXQtPanelWidget);
    mLayout->setMargin(0);
    mLayout->setSpacing(0);

    connect(QApplication::desktop(), &QDesktopWidget::resized, this, &LXQtPanel::ensureVisible);
    connect(QApplication::desktop(), &QDesktopWidget::screenCountChanged, this, &LXQtPanel::ensureVisible);

    // connecting to QDesktopWidget::workAreaResized shouldn't be necessary,
    // as we've already connceted to QDesktopWidget::resized, but it actually
    // is. Read mode on https://github.com/lxqt/lxqt-panel/pull/310
    connect(QApplication::desktop(), &QDesktopWidget::workAreaResized,
            this, &LXQtPanel::ensureVisible);

    connect(LXQt::Settings::globalSettings(), SIGNAL(settingsChanged()), this, SLOT(update()));
    connect(lxqtApp, SIGNAL(themeChanged()), this, SLOT(realign()));

    readSettings();

    ensureVisible();

    loadPlugins();

    show();
}

/************************************************

 ************************************************/
void LXQtPanel::readSettings()
{
    // Read settings ......................................
    mSettings->beginGroup(mConfigGroup);

    mScreenNum = mSettings->value(CFG_KEY_SCREENNUM, QApplication::desktop()->primaryScreen()).toInt();
    setPosition(mScreenNum, false);

    mSettings->endGroup();
}


/************************************************

 ************************************************/
void LXQtPanel::ensureVisible()
{
    if (!canPlacedOn(mScreenNum))
        setPosition(findAvailableScreen(), false);
    else
        mActualScreenNum = mScreenNum;

    // the screen size might be changed
    realign();
}


/************************************************

 ************************************************/
LXQtPanel::~LXQtPanel()
{
    mLayout->setEnabled(false);
}


/************************************************

 ************************************************/
void LXQtPanel::show()
{
    QWidget::show();
    KWindowSystem::setOnDesktop(effectiveWinId(), NET::OnAllDesktops);
}


/************************************************

 ************************************************/
void LXQtPanel::loadPlugins()
{
    mPlugins.append(new Plugin(new LXQtMainMenu(this), this));
    mPlugins.append(new Plugin(new LXQtQuickLaunchPlugin(this), this));
    mPlugins.append(new Plugin(new LXQtTaskBarPlugin(this), this));
    mPlugins.append(new Plugin(new LXQtTrayPlugin(this), this));
    mPlugins.append(new Plugin(new LXQtWorldClock(this), this));

    for (auto plugin : mPlugins)
    {
        mLayout->addWidget(plugin);
        connect(this, &LXQtPanel::realigned, plugin, &Plugin::realign);
    }

    mLayout->setStretch(2, 1);
    mLayout->insertSpacing(3, 6); /* TODO: scale with DPI */
    mLayout->insertSpacing(5, 6); /* TODO: scale with DPI */
    mLayout->insertSpacing(7, 6); /* TODO: scale with DPI */
}

/************************************************

 ************************************************/
void LXQtPanel::setPanelGeometry(bool animate)
{
    const QRect currentScreen = QApplication::desktop()->screenGeometry(mActualScreenNum);
    QRect rect;

    rect.setHeight(sizeHint().height());
    rect.setWidth(currentScreen.width());
    rect.moveLeft(currentScreen.left());
    rect.moveBottom(currentScreen.bottom());

    if (rect != geometry())
    {
        setFixedSize(rect.size());
        setMargins();
        setGeometry(rect);
    }
}

void LXQtPanel::setMargins()
{
    mLayout->setContentsMargins(0, 0, 0, 0);
}

void LXQtPanel::realign()
{
    if (!isVisible())
        return;

    setPanelGeometry();

    // Reserve our space on the screen ..........
    // It's possible that our geometry is not changed, but screen resolution is changed,
    // so resetting WM_STRUT is still needed. To make it simple, we always do it.
    updateWmStrut();
}


// Update the _NET_WM_PARTIAL_STRUT and _NET_WM_STRUT properties for the window
void LXQtPanel::updateWmStrut()
{
    WId wid = effectiveWinId();
    if(wid == 0 || !isVisible())
        return;

    const QRect wholeScreen = QApplication::desktop()->geometry();
    const QRect rect = geometry();
    // NOTE: https://standards.freedesktop.org/wm-spec/wm-spec-latest.html
    // Quote from the EWMH spec: " Note that the strut is relative to the screen edge, and not the edge of the xinerama monitor."
    // So, we use the geometry of the whole screen to calculate the strut rather than using the geometry of individual monitors.
    // Though the spec only mention Xinerama and did not mention XRandR, the rule should still be applied.
    // At least openbox is implemented like this.
    KWindowSystem::setExtendedStrut(wid,
                                    /* Left   */  0, 0, 0,
                                    /* Right  */  0, 0, 0,
                                    /* Top    */  0, 0, 0,
                                    /* Bottom */  wholeScreen.bottom() - rect.bottom() + height(), rect.left(), rect.right()
                                   );
}


/************************************************
  The panel can't be placed on boundary of two displays.
  This function checks if the panel can be placed on the display
  @screenNum on @position.
 ************************************************/
bool LXQtPanel::canPlacedOn(int screenNum)
{
    QDesktopWidget* dw = QApplication::desktop();

    for (int i = 0; i < dw->screenCount(); ++i)
        if (dw->screenGeometry(i).top() > dw->screenGeometry(screenNum).bottom())
            return false;
    return true;
}


/************************************************

 ************************************************/
int LXQtPanel::findAvailableScreen()
{
    int current = mScreenNum;

    for (int i = current; i < QApplication::desktop()->screenCount(); ++i)
        if (canPlacedOn(i))
            return i;

    for (int i = 0; i < current; ++i)
        if (canPlacedOn(i))
            return i;

    return 0;
}


/************************************************

 ************************************************/
void LXQtPanel::setPosition(int screen, bool save)
{
    if (mScreenNum == screen)
        return;

    mActualScreenNum = screen;

    if (save)
    {
        mScreenNum = screen;
    }

    // Qt 5 adds a new class QScreen and add API for setting the screen of a QWindow.
    // so we had better use it. However, without this, our program should still work
    // as long as XRandR is used. Since XRandR combined all screens into a large virtual desktop
    // every screen and their virtual siblings are actually on the same virtual desktop.
    // So things still work if we don't set the screen correctly, but this is not the case
    // for other backends, such as the upcoming wayland support. Hence it's better to set it.
    if(windowHandle())
    {
        // QScreen* newScreen = qApp->screens().at(screen);
        // QScreen* oldScreen = windowHandle()->screen();
        // const bool shouldRecreate = windowHandle()->handle() && !(oldScreen && oldScreen->virtualSiblings().contains(newScreen));
        // Q_ASSERT(shouldRecreate == false);

        // NOTE: When you move a window to another screen, Qt 5 might recreate the window as needed
        // But luckily, this never happen in XRandR, so Qt bug #40681 is not triggered here.
        // (The only exception is when the old screen is destroyed, Qt always re-create the window and
        // this corner case triggers #40681.)
        // When using other kind of multihead settings, such as Xinerama, this might be different and
        // unless Qt developers can fix their bug, we have no way to workaround that.
        windowHandle()->setScreen(qApp->screens().at(screen));
    }

    realign();
}


/************************************************

 ************************************************/
QRect LXQtPanel::globalGeometry() const
{
    // panel is the the top-most widget/window, no calculation needed
    return geometry();
}


/************************************************

 ************************************************/
bool LXQtPanel::event(QEvent *event)
{
    switch (event->type())
    {
    case QEvent::LayoutRequest:
        emit realigned();
        break;

    case QEvent::WinIdChange:
    {
        // qDebug() << "WinIdChange" << hex << effectiveWinId();
        if(effectiveWinId() == 0)
            break;

        // Sometimes Qt needs to re-create the underlying window of the widget and
        // the winId() may be changed at runtime. So we need to reset all X11 properties
        // when this happens.
        qDebug() << "WinIdChange" << hex << effectiveWinId() << "handle" << windowHandle() << windowHandle()->screen();

        // Qt::WA_X11NetWmWindowTypeDock becomes ineffective in Qt 5
        // See QTBUG-39887: https://bugreports.qt-project.org/browse/QTBUG-39887
        // Let's use KWindowSystem for that
        KWindowSystem::setType(effectiveWinId(), NET::Dock);

        updateWmStrut(); // reserve screen space for the panel
        KWindowSystem::setOnAllDesktops(effectiveWinId(), true);
        break;
    }

    default:
        break;
    }

    return QFrame::event(event);
}

/************************************************

 ************************************************/

void LXQtPanel::showEvent(QShowEvent *event)
{
    QFrame::showEvent(event);
    realign();
}

Plugin* LXQtPanel::findPlugin(const ILXQtPanelPlugin* iPlugin) const
{
    for (auto plug : mPlugins)
        if (plug->iPlugin() == iPlugin)
            return plug;
    return nullptr;
}

/************************************************

 ************************************************/
QRect LXQtPanel::calculatePopupWindowPos(QPoint const & absolutePos, QSize const & windowSize) const
{
    int x = absolutePos.x();
    int y = globalGeometry().top() - windowSize.height();

    QRect res(QPoint(x, y), windowSize);

    QRect screen = QApplication::desktop()->screenGeometry(this);
    // NOTE: We cannot use AvailableGeometry() which returns the work area here because when in a
    // multihead setup with different resolutions. In this case, the size of the work area is limited
    // by the smallest monitor and may be much smaller than the current screen and we will place the
    // menu at the wrong place. This is very bad for UX. So let's use the full size of the screen.
    if (res.right() > screen.right())
        res.moveRight(screen.right());

    if (res.bottom() > screen.bottom())
        res.moveBottom(screen.bottom());

    if (res.left() < screen.left())
        res.moveLeft(screen.left());

    if (res.top() < screen.top())
        res.moveTop(screen.top());

    return res;
}

/************************************************

 ************************************************/
QRect LXQtPanel::calculatePopupWindowPos(const ILXQtPanelPlugin *plugin, const QSize &windowSize) const
{
    Plugin *panel_plugin = findPlugin(plugin);
    if (nullptr == panel_plugin)
    {
        qWarning() << Q_FUNC_INFO << "Wrong logic? Unable to find Plugin* for" << plugin << "known plugins follow...";
        for (auto plug : mPlugins)
            qWarning() << plug->iPlugin() << plug;

        return QRect();
    }

    // Note: assuming there are not contentMargins around the "BackgroundWidget" (LXQtPanelWidget)
    return calculatePopupWindowPos(globalGeometry().topLeft() + panel_plugin->geometry().topLeft(), windowSize);
}


/************************************************

 ************************************************/
void LXQtPanel::userRequestForDeletion()
{
    const QMessageBox::StandardButton ret
        = QMessageBox::warning(this, tr("Remove Panel", "Dialog Title") ,
            tr("Removing a panel can not be undone.\nDo you want to remove this panel?"),
            QMessageBox::Yes | QMessageBox::No);

    if (ret != QMessageBox::Yes) {
        return;
    }

    mSettings->beginGroup(mConfigGroup);
    const QStringList plugins = mSettings->value("plugins").toStringList();
    mSettings->endGroup();

    for(const QString& i : plugins)
        if (!i.isEmpty())
            mSettings->remove(i);

    mSettings->remove(mConfigGroup);

    emit deletedByUser(this);
}
