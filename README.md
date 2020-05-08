qmpanel - A Minimal Qt-Based Desktop Panel
==========================================

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

    - System tray (XEmbed)
       - Icons sorted by window class

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
```
