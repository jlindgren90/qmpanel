set(PLUGIN "quicklaunch")

set(HEADERS
    quicklaunch.h
)

set(SOURCES
    quicklaunch.cpp
)

set(LIBRARIES
    Qt5Xdg
)

include_directories(
    ${LXQT_INCLUDE_DIRS}
)

BUILD_LXQT_PLUGIN(${PLUGIN})
