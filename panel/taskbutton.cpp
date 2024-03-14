/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * qmpanel - a minimal Qt-based desktop panel
 *
 * Copyright: 2011 Razor team
 *            2014 LXQt team
 *            2020-2024 John Lindgren
 * Authors:
 *   Alexander Sokoloff <sokoloff.a@gmail.com>
 *   Kuzma Shapran <kuzma.shapran@gmail.com>
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

#include "taskbutton.h"
#include "resources.h"
#include "wlr-foreign-toplevel-management-unstable-v1.h"

#include <KWindowInfo>
#include <KX11Extras>
#include <NETWM>
#include <QDragEnterEvent>
#include <QGuiApplication>
#include <QStyle>
#include <QTimer>
#include <private/qtx11extras_p.h>

TaskButton::TaskButton(QWidget * parent) : QToolButton(parent)
{
    setCheckable(true);
    setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    setAcceptDrops(true);

    mTimer.setSingleShot(true);
    mTimer.setInterval(500);

    connect(this, &QToolButton::clicked, [this](bool checked) {
        if (checked)
            activateWindow();
        else
            minimizeWindow();
    });

    connect(&mTimer, &QTimer::timeout, this, &TaskButton::activateWindow);
}

QSize TaskButton::sizeHint() const
{
    return {2 * logicalDpiX(), QToolButton::sizeHint().height()};
}

void TaskButton::dragEnterEvent(QDragEnterEvent * event)
{
    mTimer.start();
    event->acceptProposedAction();
    QToolButton::dragEnterEvent(event);
}

void TaskButton::dragLeaveEvent(QDragLeaveEvent * event)
{
    mTimer.stop();
    QToolButton::dragLeaveEvent(event);
}

void TaskButton::dropEvent(QDropEvent * event)
{
    mTimer.stop();
    QToolButton::dropEvent(event);
}

void TaskButton::mousePressEvent(QMouseEvent * event)
{
    if (event->button() == Qt::MiddleButton)
    {
        closeWindow();
        event->accept();
        return;
    }

    QToolButton::mousePressEvent(event);
}

TaskButtonX11::TaskButtonX11(const WId window, QWidget * parent)
    : TaskButton(parent), mWindow(window)
{
    updateText();
    updateIcon();

    if (KX11Extras::activeWindow() == window)
        setChecked(true);
}

void TaskButtonX11::updateText()
{
    KWindowInfo info(mWindow, NET::WMVisibleName | NET::WMName);
    QString title = info.visibleName();
    if (title.isEmpty())
        title = info.name();

    setText(title.replace("&", "&&"));
    setToolTip(title);
}

void TaskButtonX11::updateIcon()
{
    int size = style()->pixelMetric(QStyle::PM_ToolBarIconSize);
    size *= devicePixelRatioF();
    QIcon icon = KX11Extras::icon(mWindow, size, size);
    if (icon.isNull())
        icon = style()->standardIcon(QStyle::SP_FileIcon);
    setIcon(icon);
}

void TaskButtonX11::activateWindow()
{
    KX11Extras::forceActiveWindow(mWindow);
    // need to flush if called from timer
    xcb_flush(QX11Info::connection());
}

void TaskButtonX11::minimizeWindow() { KX11Extras::minimizeWindow(mWindow); }

void TaskButtonX11::closeWindow()
{
    NETRootInfo info(QX11Info::connection(), NET::CloseWindow);
    info.closeWindowRequest(mWindow);
}

TaskButtonWayland::TaskButtonWayland(Resources & res,
                                     zwlr_foreign_toplevel_handle_v1 * handle,
                                     QWidget * parent)
    : TaskButton(parent), mRes(res), mHandle(handle)
{
    static const zwlr_foreign_toplevel_handle_v1_listener toplevel_handle_impl =
        {
            .title =
                [](void * data, zwlr_foreign_toplevel_handle_v1 * handle,
                   const char * title) {
                    auto self = static_cast<TaskButtonWayland *>(data);
                    self->setText(QString(title).replace("&", "&&"));
                    self->setToolTip(title);
                },
            .app_id =
                [](void * data, zwlr_foreign_toplevel_handle_v1 * handle,
                   const char * app_id) {
                    static_cast<TaskButtonWayland *>(data)->setAppName(app_id);
                },
            .output_enter =
                [](void * data, zwlr_foreign_toplevel_handle_v1 * handle,
                   wl_output * output) {
                    /* no-op */
                },
            .output_leave =
                [](void * data, zwlr_foreign_toplevel_handle_v1 * handle,
                   wl_output * output) {
                    /* no-op */
                },
            .state =
                [](void * data, zwlr_foreign_toplevel_handle_v1 * handle,
                   wl_array * state) {
                    auto start = static_cast<const uint32_t *>(state->data);
                    auto end = start + (state->size / sizeof(uint32_t));
                    bool activated =
                        (std::find(
                             start, end,
                             ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_ACTIVATED) !=
                         end);
                    static_cast<TaskButtonWayland *>(data)->setChecked(
                        activated);
                },
            .done =
                [](void * data, zwlr_foreign_toplevel_handle_v1 * handle) {
                    /* no-op */
                },
            .closed =
                [](void * data, zwlr_foreign_toplevel_handle_v1 * handle) {
                    auto self = static_cast<TaskButtonWayland *>(data);
                    self->deleteLater();
                },
            .parent =
                [](void * data, zwlr_foreign_toplevel_handle_v1 * handle,
                   zwlr_foreign_toplevel_handle_v1 * parent) {
                    /* no-op */
                },
        };

    zwlr_foreign_toplevel_handle_v1_add_listener(handle, &toplevel_handle_impl,
                                                 this);

    // set default icon (usually changed from app_id callback)
    setIcon(style()->standardIcon(QStyle::SP_FileIcon));
}

TaskButtonWayland::~TaskButtonWayland()
{
    zwlr_foreign_toplevel_handle_v1_destroy(mHandle);
}

void TaskButtonWayland::activateWindow()
{
    auto waylandApp =
        qGuiApp->nativeInterface<QNativeInterface::QWaylandApplication>();
    zwlr_foreign_toplevel_handle_v1_unset_minimized(mHandle);
    zwlr_foreign_toplevel_handle_v1_activate(mHandle, waylandApp->seat());
}

void TaskButtonWayland::minimizeWindow()
{
    zwlr_foreign_toplevel_handle_v1_set_minimized(mHandle);
}

void TaskButtonWayland::closeWindow()
{
    zwlr_foreign_toplevel_handle_v1_close(mHandle);
}

void TaskButtonWayland::setAppName(const QString & appName)
{
    auto icon = mRes.getAppIcon(appName);
    if (!icon.isNull())
        setIcon(icon);
}
