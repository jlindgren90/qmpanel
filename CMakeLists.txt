cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)
project(qmpanel)

find_package(Threads REQUIRED)
find_package(KF5WindowSystem REQUIRED)
find_package(PkgConfig REQUIRED)

pkg_check_modules(X11 REQUIRED x11 xcomposite xdamage xrender)
pkg_check_modules(XCB REQUIRED xcb xcb-util xcb-damage)
pkg_check_modules(GIO REQUIRED gio-2.0 gio-unix-2.0)
pkg_check_modules(QT REQUIRED Qt5Widgets Qt5X11Extras)

add_subdirectory(panel)
