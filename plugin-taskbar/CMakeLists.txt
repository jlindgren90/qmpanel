set(PLUGIN "taskbar")

set(HEADERS
    taskbar.h
    taskbutton.h
)

set(SOURCES
    taskbar.cpp
    taskbutton.cpp
)

BUILD_LXQT_PLUGIN(${PLUGIN})
