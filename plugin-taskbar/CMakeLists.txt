set(PLUGIN "taskbar")

set(HEADERS
    lxqttaskbar.h
    lxqttaskbutton.h
    lxqttaskbarplugin.h
    lxqttaskgroup.h
    lxqtgrouppopup.h
)

set(SOURCES
    lxqttaskbar.cpp
    lxqttaskbutton.cpp
    lxqttaskbarplugin.cpp
    lxqttaskgroup.cpp
    lxqtgrouppopup.cpp
)

set(LIBRARIES
    lxqt
    lxqt-globalkeys
    Qt5Xdg
)

BUILD_LXQT_PLUGIN(${PLUGIN})
