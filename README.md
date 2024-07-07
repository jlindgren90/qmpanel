qmpanel - A Minimal Qt-Based Desktop Panel
==========================================

![screenshot](/qmpanel.png?raw=true)

## What is this?

 - A standalone desktop panel with a simple, traditional design
 - Lightly customizable, but with a "Just Works" mentality
 - Originally a fork of lxqt-panel

## Features

 - Searchable applications menu
 - Configurable quick-launch toolbar
 - Taskbar (showing running applications)
 - Status icon area ("system tray")
 - Date & time display with pop-up calendar

## Where to use qmpanel

Currently, qmpanel works best under X11, for example with
[Openbox](http://openbox.org/wiki/Main_Page).

Wayland support is currently experimental, and you *will* encounter bugs
due to the immaturity of Wayland in general. If you wish to use Wayland
anyway, I recommend using [labwc](https://github.com/labwc/labwc/). Make
sure to use the latest versions of Qt and labwc.

Some known Wayland issues affecting qmpanel:

 - Popup window repositioning has only recently been implemented
   [in Qt](https://codereview.qt-project.org/c/qt/qtwayland/+/481718)
   and [in labwc](https://github.com/labwc/labwc/pull/1950). Without
   these changes, the applications menu will not be positioned correctly
   when searching.

 - Nested menus are also positioned incorrectly due to a known
   [Qt issue](https://bugreports.qt.io/browse/QTBUG-124810).

## Getting qmpanel

In Arch Linux, qmpanel is available via the AUR. On other systems, you
can clone the repository and compile from source.

First ensure you have the required dependencies installed:

 - Qt 6.5+
 - GLib 2.32+
 - KWindowSystem 6.0+
 - LayerShellQt 6.0+
 - meson (build dependency)
 - a C++ compiler (such as GCC)

To build, run `meson setup build && meson compile -C build`.

Then simply run `./build/qmpanel`. No installation is necessary.

## Configuration (optional)

For default settings, you can run qmpanel with no configuration at all.

To customize it, first create the file `$HOME/.config/qmpanel.ini`. The
file contents should match the following format:

    [Settings]
    # Sets the icon displayed for the applications menu
    MenuIcon=<icon-name>
    # Pins favorite applications to the top of the menu
    PinnedMenuApps=<app-name>.desktop;<app-name>.desktop
    # Adds applications to the quick-launch toolbar
    QuickLaunchApps=<app-name>.desktop;<app-name>.desktop
    # Runs commands (e.g. system tray icons) at startup
    LaunchCmds=<command>;<command>

All lines except the first (`[Settings]`) are optional.

## Design philosophy

 - Stay small, value correctness above features
 - Work "out of the box" with minimal configuration
 - Depend only on widely-used libraries (GLib, Qt, KWindowSystem, etc.)

## Potential future development

 - (External) fix remaining Qt issues under Wayland
 - Upstream libdbusmenu-qt changes (or find a replacement)
 - Non-English translations

Features _not_ planned:

 - Plugins/extensions
 - Custom colors/themes (beyond the Qt theme)
 - Custom panel layouts or multiple panels
 - Grouping/ordering of taskbar buttons

## Version history

### 0.1 (8 May 2020)

 - Initial release

### 0.2 (2 Oct 2023)

 - Build system changed from CMake to meson
 - System tray changed from XEmbed to StatusNotifierItem
 - New `LaunchCmds` feature to start system tray icons
 - Several bug fixes (refer to commit log)

### 0.3 (21 Dec 2023)

 - Several bug fixes to StatusNotifierItem system tray
 - Work around multi-monitor issues under XWayland

### 0.4 (25 Mar 2024)

 - Port to Qt 6 and KWindowSystem 6
 - Experimental native Wayland support using LayerShellQt

### 0.5 (7 Jul 2024)

 - Fix various Wayland-related issues
