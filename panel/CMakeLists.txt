set(PROJECT lxqt-panel)

set(SOURCES
    actionview.cpp
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
    ${XCOMPOSITE_LDFLAGS}
    ${XDAMAGE_LIBRARIES}
    ${XRENDER_LIBRARIES}
    lxqt
)

project(${PROJECT})
add_executable(${PROJECT} ${SOURCES})
target_link_libraries(${PROJECT} ${LIBRARIES})
install(TARGETS ${PROJECT} RUNTIME DESTINATION bin)
