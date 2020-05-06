set(PROJECT lxqt-panel)

set(SOURCES
    actionview.cpp
    appdb.cpp
    clocklabel.cpp
    main.cpp
    mainmenu.cpp
    lxqtpanel.cpp
    quicklaunch.cpp
    systray.cpp
    taskbar.cpp
    taskbutton.cpp
    trayicon.cpp
)

set(LIBRARIES
    ${CMAKE_THREAD_LIBS_INIT}
    Qt5::Widgets
    KF5::WindowSystem
    ${X11_LIBRARIES}
    ${XCB_LIBRARIES}
    ${XCOMPOSITE_LIBRARIES}
    ${XDAMAGE_LIBRARIES}
    ${XRENDER_LIBRARIES}
    ${GIO_LIBRARIES}
    lxqt
)

include_directories(${GIO_INCLUDE_DIRS})

project(${PROJECT})
add_executable(${PROJECT} ${SOURCES})
target_link_libraries(${PROJECT} ${LIBRARIES})
install(TARGETS ${PROJECT} RUNTIME DESTINATION bin)
