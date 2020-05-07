set(PROJECT qmpanel)

set(SOURCES
    actionview.cpp
    appdb.cpp
    clocklabel.cpp
    main.cpp
    mainmenu.cpp
    mainpanel.cpp
    quicklaunch.cpp
    systray.cpp
    taskbar.cpp
    taskbutton.cpp
    trayicon.cpp
)

set(LIBRARIES
    ${CMAKE_THREAD_LIBS_INIT}
    KF5::WindowSystem
    ${X11_LIBRARIES}
    ${XCB_LIBRARIES}
    ${GIO_LIBRARIES}
    ${QT_LIBRARIES}
)

include_directories(
    ${GIO_INCLUDE_DIRS}
    ${QT_INCLUDE_DIRS}
)

project(${PROJECT})
add_executable(${PROJECT} ${SOURCES})
target_link_libraries(${PROJECT} ${LIBRARIES})
install(TARGETS ${PROJECT} RUNTIME DESTINATION bin)
