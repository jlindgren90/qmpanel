set(PLUGIN "taskbar")

set(HEADERS
    lxqttaskbar.h
    lxqttaskbutton.h
)

set(SOURCES
    lxqttaskbar.cpp
    lxqttaskbutton.cpp
)

set(LIBRARIES
    Qt5Xdg
)

BUILD_LXQT_PLUGIN(${PLUGIN})
