qmpanel - A Minimal Qt-Based Desktop Panel
==========================================

![screenshot](/qmpanel.png?raw=true)

 - Origin: fork of lxqt-panel 0.14.1

 - Reasons for fork:

    - Feature creep and number of bugs in lxqt-panel and LXQt libraries
    - Small number of lxqt-panel developers with other priorities

 - Design goals:

    - Stay small, value correctness above features
    - Work "out of the box" with minimal configuration
    - Follow Qt settings from environment (no custom colors/themes)
    - Depend only on widely-used libraries (Qt, KWindowSystem, GIO)
    - No optional parts (build options, loadable plugins)
    - Support only US English initially

 - Type of panel:

    - Single panel with a traditional, fixed layout
    - Always displayed at bottom of primary monitor
    - Height automatically computed

 - Panel items:

    - Applications menu
       - Icon-only QToolButton, icon configurable
       - Predefined application category sub-menus
       - Configurable list of applications "pinned" to top of menu
       - Integrated application search at bottom of menu

    - Quick launch
       - Configurable list of applications
       - Icon-only QToolButtons
       - Tooltip shows application name

    - Task bar / window list
       - Buttons with text beside icon
       - Tooltip shows full window title
       - Sorted by creation time (no drag to reorder)
       - No window grouping
       - Left-click to show/minimize, middle-click to close
       - Activate window by hover during drag-and-drop

    - Expanding space

    - System tray (StatusNotifierItem)
       - Uses KDE's fork of libdbusmenu-qt (same as in plasma-workspace)
       - Some less common StatusNotifierItem features not supported
       - Icons sorted by title

    - Date and time
       - e.g. "Thu Apr 23, 2:25 pm"
       - click to show/hide a QCalendarWidget

 - Configuration:

    - No configuration GUI
    - Settings are read from $HOME/.config/qmpanel.ini
    - Example qmpanel.ini:

```
[Settings]
MenuIcon=<icon-name>
PinnedMenuApps=<app-name>.desktop;<app-name>.desktop
QuickLaunchApps=<app-name>.desktop;<app-name>.desktop
# LaunchCmds are executed at startup, after the StatusNotifierItem D-Bus
# service is registered. You can start system tray applications here.
LaunchCmds=<command>;<command>
```

 - Version history:

    - 0.1 (8 May 2020)
       - Initial release

    - 0.2 (2 Oct 2023)
       - Build system changed from CMake to meson
       - System tray changed from XEmbed to StatusNotifierItem
       - New `LaunchCmds` feature to start system tray icons
       - Several bug fixes (refer to commit log)
