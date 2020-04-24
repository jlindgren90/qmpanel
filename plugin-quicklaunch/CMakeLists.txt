set(PLUGIN "quicklaunch")

set(HEADERS
    lxqtquicklaunch.h
)

set(SOURCES
    lxqtquicklaunch.cpp
)

set(LIBRARIES
    Qt5Xdg
)

include_directories(
    ${LXQT_INCLUDE_DIRS}
)

BUILD_LXQT_PLUGIN(${PLUGIN})
