set(PLUGIN "mainmenu")

set(HEADERS
    actionview.h
    mainmenu.h
)

set(SOURCES
    actionview.cpp
    mainmenu.cpp
)

set(LIBRARIES
    lxqt
    lxqt-globalkeys
    lxqt-globalkeys-ui
)

set(QT_USE_QTXML 1)
set(QT_USE_QTDBUS 1)

BUILD_LXQT_PLUGIN(${PLUGIN})
