set(PLUGIN "taskbar")

set(HEADERS
    lxqttaskbar.h
    lxqttaskbutton.h
    lxqttaskbarplugin.h
)

set(SOURCES
    lxqttaskbar.cpp
    lxqttaskbutton.cpp
    lxqttaskbarplugin.cpp
)

set(LIBRARIES
    lxqt
    lxqt-globalkeys
    Qt5Xdg
)

BUILD_LXQT_PLUGIN(${PLUGIN})
